/**
*   Eva Virtual Machine
*/

#ifndef __EvaVM_h
#define __EvaVM_h

#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <stack>

#include "../Logger.h"
#include "../bytecode/OpCode.h"
#include "../compiler/EvaCompiler.h"
#include "../parser/EvaParser.h"
#include "EvaValue.h"
#include "Global.h"

using syntax::EvaParser;

#define READ_BYTE() *ip++

#define READ_SHORT() (ip += 2, (uint16_t) ((ip[-2] << 8) | ip[-1]))

#define TO_ADDRESS(index) (&fn->co->code[index])

#define GET_CONST() (fn->co->constants[READ_BYTE()])

#define STACK_LIMIT 512

/**
 * Generic binary operation
 */ 
#define BINARY_OP(op) \
    do { \
        auto op2 = AS_NUMBER(pop()); \
        auto op1 = AS_NUMBER(pop()); \
        push(NUMBER(op1 op op2)); \
    } while (false)

/**
 * Generic values comparison
 */ 
#define COMPARE_VALUES(op, v1, v2)      \
    do {                                \
        bool res;                       \
        switch (op) {                   \
            case 0:                     \
                res = v1 < v2;          \
                break;                  \
            case 1:                     \
                res = v1 > v2;          \
                break;                  \
            case 2:                     \
                res = v1 == v2;         \
                break;                  \
            case 3:                     \
                res = v1 >= v2;         \
                break;                  \
            case 4:                     \
                res = v1 <= v2;         \
                break;                  \
            case 5:                     \
                res = v1 != v2;         \
                break;                  \
        }                               \
        push(BOOLEAN(res));             \
    } while (false)

// --------------------------------------------------------------
/**
 * Stack frame for function calls.
 */ 
struct Frame {
    /**
     * Return address of the caller (ip of the caller)
     */ 
    uint8_t* ra;

    /**
     * Base pointer of the caller
     */ 
    EvaValue* bp;

    /**
     * Reference to the running function:
     * contains code, locals, etc.
     */ 
    FunctionObject* fn;
};


// --------------------------------------------------------------
class EvaVM {
    public:
        EvaVM() 
            :   global(std::make_shared<Global>()),
                parser(std::make_unique<EvaParser>()),
                compiler(std::make_unique<EvaCompiler>(global)) {
                    setGlobalVariables();
                }

            
        void push(const EvaValue& value) {
            if ((size_t) (sp - stack.begin()) == STACK_LIMIT) {
                DIE << "push(): Stack overflow. \n";
            }
            *sp = value;
            sp++;
        }

        EvaValue pop() {
            if (sp == stack.begin()) {
                DIE << "pop(): empty stack. \n";
            }
            --sp;
            return *sp;
        }

        EvaValue peek(size_t offset = 0) {
            if (stack.size() == 0) {
                DIE << "pop(): empty stack. \n";
            }
            return *(sp - 1 - offset);
        }

        void popN(size_t count) {
            if (stack.size() == 0) {
                DIE << "popN(): empty stack. \n";
            }
            sp -= count;
        }

    EvaValue exec(const std::string &program) {
        // 1. Parse to AST
        auto ast = parser->parse("(begin " + program + ")");

        // 2. Compile to Bytecode
        compiler->compile(ast);

        //Start from the main entry point:
        fn = compiler->getMainFunction();

        // Set IP to beginning
        ip = &fn->co->code[0];

        // Initialize stack
        sp = &stack[0];

        // Initialize base/frame pointer:
        bp = sp;

        // Debug disassembly
        compiler->disassembleBytecode();

        return eval();
    }

    /**
     * Main Eval Loop
     */
    EvaValue eval() {
        for(;;) {
            // dumpStack();
            int opcode = READ_BYTE();
            switch(opcode) {
                case OP_HALT: {
                    return pop();
                }
                case OP_CONST: {
                    push(GET_CONST());
                    break;
                }
                case OP_ADD: {
                    auto op2 = pop();
                    auto op1 = pop();

                    // Numeric addition:
                    if (IS_NUMBER(op1) && IS_NUMBER(op2)) {
                        auto v1 = AS_NUMBER(op1);
                        auto v2 = AS_NUMBER(op2);
                        push(NUMBER(v1 + v2));
                    }

                    // String addition:
                    if (IS_STRING(op1) && IS_STRING(op2)) {
                        auto s1 = AS_CPPSTRING(op1);
                        auto s2 = AS_CPPSTRING(op2);
                        push(ALLOC_STRING(s1 + s2));
                    }
                    break;
                }
                case OP_SUB: {
                    BINARY_OP(-);
                    break;
                }
                case OP_MUL: {
                    BINARY_OP(*);
                    break;
                }
                case OP_DIV: {
                    BINARY_OP(/);
                    break;
                }
                // Comparison
                case OP_COMPARE: {
                    auto op = READ_BYTE();
                    auto op2 = pop();
                    auto op1 = pop();
                    if (IS_NUMBER(op1) && IS_NUMBER(op2)) {
                        auto v1 = AS_NUMBER(op1);
                        auto v2 = AS_NUMBER(op2);
                        COMPARE_VALUES(op, v1, v2);
                    } else if (IS_STRING(op1) && IS_STRING(op2)) {
                        auto s1 = AS_CPPSTRING(op1);
                        auto s2 = AS_CPPSTRING(op2);
                        COMPARE_VALUES(op, s1, s2);
                    }
                    break;
                }

                // Conditional jump:
                case OP_JMP_IF_FALSE: {
                    auto cond = AS_BOOLEAN(pop()); // TODO: to boolean, 0->false etc

                    auto address = READ_SHORT();

                    if (!cond) {
                        ip = TO_ADDRESS(address);
                    }

                    break;
                }

                // Conditional jump:
                case OP_JMP: {
                    ip = TO_ADDRESS(READ_SHORT());
                    break;
                }

                // Global variable value:
                case OP_GET_GLOBAL: {
                    auto globalIndex = READ_BYTE();
                    push(global->get(globalIndex).value);
                    break;
                }

                case OP_SET_GLOBAL: {
                    auto globalIndex = READ_BYTE();
                    auto value = peek(0);
                    global->set(globalIndex, value);
                    break;
                }

                // Stack manipulation
                case OP_POP: {
                    pop();
                    break;
                }

                // Local variable value
                case OP_GET_LOCAL: {
                    auto localIndex = READ_BYTE();
                    if (localIndex < 0 || localIndex >= stack.size()) {
                        DIE << "OP_GET_LOCAL: invalid variabel index: " << (int)localIndex;
                    }
                    push(bp[localIndex]);
                    break;
                }

                // Local variable value
                case OP_SET_LOCAL: {
                    auto localIndex = READ_BYTE();
                    auto value = peek(0);
                    if (localIndex < 0 || localIndex >= stack.size()) {
                        DIE << "OP_SET_LOCAL: invalid variable index: " << (int)localIndex;
                    }
                    bp[localIndex] = value;
                    break;
                }
                //----------------------------------------------
                // Scope Exit
                //
                // Note: variables sit right below the result of a block,
                // so we move the result below, which will be the new top
                // after popping the variables
                case OP_SCOPE_EXIT: {
                    // How many vars to pop:
                    auto count = READ_BYTE();

                    // Move result above the vars:
                    *(sp - 1 - count) = peek(0);

                    // Pop the vars:
                    popN(count);
                    break;
                }

                case OP_CALL: {
                    auto argsCount = READ_BYTE();
                    auto fnValue = peek(argsCount);

                    // 1. Native function:
                    if (IS_NATIVE(fnValue)) {
                        AS_NATIVE(fnValue) ->function();
                        auto result = pop();

                        // Pop args and function
                        popN(argsCount + 1);

                        // Put result back on top
                        push(result);
                        break;
                    }

                    // 2. User-defined function TODO
                    auto callee = AS_FUNCTION(fnValue);

                    // save execution context, restored on OP_RETURN
                    callStack.push(Frame{ip, bp, fn});

                    // To access locals, etc:
                    fn = callee;

                    // Set the base (frame) pointer for the callee:
                    bp = sp - argsCount - 1;

                    // Jump to the function code
                    ip = &callee->co->code[0];

                    break;
                }

                case OP_RETURN: {
                    // Restore the caller address
                    auto callerFrame = callStack.top();

                    // Restore stack pointers:
                    ip = callerFrame.ra;
                    bp = callerFrame.bp;
                    fn = callerFrame.fn;

                    callStack.pop();
                    break;
                }

                default: {
                    DIE << "Unknown opcode: " << std::hex << opcode;
                }
            }
        }    
    }

    /**
     * Sets up global variables and function.
     */
    void setGlobalVariables() {
        global->addNativeFunction(
            "native-square",
            [&]() {
                auto x = AS_NUMBER(peek(0));
                push(NUMBER(x * x));
            },
            1);

        global->addNativeFunction(
            "native-sum",
            [&]() {
                auto v2 = AS_NUMBER(peek(0));
                auto v1 = AS_NUMBER(peek(1));
                push(NUMBER(v1 + v2));
            },
            2);

        global->addConst("y", 20);
    }

    /**
     * Global object
     */
    std::shared_ptr<Global> global;

    /**
     * Parser.
     */
    std::unique_ptr<EvaParser> parser;

    /**
     * Compiler.
     */
    std::unique_ptr<EvaCompiler> compiler;

    /**
     * Instruction Pointer
     */ 
    uint8_t* ip;

    /**
     * Stack pointer
     */ 
    EvaValue* sp;

    /**
     * Base pointer (aka Frame pointer)
     */ 
    EvaValue* bp;

    /**
     * Operands Stack
     */ 
    std::array<EvaValue, STACK_LIMIT> stack;

    /**
     * Separate stack for calls.  Keeps return addresses.
     */
    std::stack<Frame> callStack;

    /**
     * Code object
     */ 
    FunctionObject* fn;

    //-----------------------------------------------
    //  Debug functions:

    /**
     *  Dumps the current stack.
     */ 
    void dumpStack() {
        std::cout << "\n-----------Stack-----------\n";
        if (sp == stack.begin()) {
            std::cout << "(empty)";
        }
        auto csp = sp - 1;
        while (csp >= stack.begin()) {
            std::cout << *csp-- << "\n";
        }
        std::cout << "\n";
    };
};

#endif

