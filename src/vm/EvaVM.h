/**
*   Eva Virtual Machine
*/

#ifndef __EvaVM_h
#define __EvaVM_h

#include <string>
#include <vector>
#include <array>
#include <iostream>

#include "../Logger.h"
#include "../bytecode/OpCode.h"
#include "../compiler/EvaCompiler.h"
#include "../parser/EvaParser.h"
#include "EvaValue.h"

using syntax::EvaParser;

#define READ_BYTE() *ip++

#define GET_CONST() co->constants[READ_BYTE()]

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


class EvaVM {
    public:
        // EvaVM() : parser(std::make_unique<EvaParser>) {}
        // const std::unique_ptr<EvaParser> parser;

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

    EvaValue exec(const std::string &program) {
        // TODO: Instantiate with unique_ptr instead of new()
        EvaParser* parser = new EvaParser();
        EvaCompiler* compiler = new EvaCompiler();


        // 1. Parse to AST
        auto ast = parser->parse(program);

        // 2. Compile to Bytecode
        co = compiler->compile(ast);
        // constants.push_back(ALLOC_STRING("Hello, "));
        // constants.push_back(ALLOC_STRING("world!"));
        // code = {OP_CONST, 0, OP_CONST, 1, OP_ADD, OP_HALT};

        // Set IP to beginning
        ip = &co->code[0];

        // Initialize stack
        sp = &stack[0];

        return eval();
    }

    /**
     * Main Eval Loop
     */
    EvaValue eval() {
        for(;;) {
            int opcode = READ_BYTE();
            log(opcode);
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

                default: {
                    DIE << "Unknown opcode: " << std::hex << opcode;
                }
            }
        }    
    }
    
    /**
     * Parser
     */ 
    const std::unique_ptr<EvaParser> parser;


    /**
     * Instruction Pointer
     */ 
    uint8_t* ip;

    /**
     * Stack pointer
     */ 
    EvaValue* sp;

    /**
     * Operands Stack
     */ 
    std::array<EvaValue, STACK_LIMIT> stack;

    /**
     * Code object
     */ 
    CodeObject* co;
};

#endif

