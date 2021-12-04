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
#include "EvaValue.h"

#define READ_BYTE() *ip++

#define GET_CONST() constants[READ_BYTE()]

#define STACK_LIMIT 512

#define BINARY_OP(op) \
    do { \
        auto op2 = AS_NUMBER(pop()); \
        auto op1 = AS_NUMBER(pop()); \
        push(NUMBER(op1 op op2)); \
    } while (false)


class EvaVM {
    public:
        EvaVM() {}

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
        // 1. Parse to AST

        // 2. Compile to Bytecode
        constants.push_back(ALLOC_STRING("Hello, "));
        constants.push_back(ALLOC_STRING("world!"));
        code = {OP_CONST, 0, OP_CONST, 1, OP_ADD, OP_HALT};

        // Set IP to beginning
        ip = &code[0];

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
                default: {
                    DIE << "Unknown opcode: " << std::hex << opcode;
                }
            }
        }    
    }

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
     * Constant Pool
     */ 
    std::vector<EvaValue> constants;

    /**
     *  Bytecode
     */
    std::vector<uint8_t> code;
};

#endif

