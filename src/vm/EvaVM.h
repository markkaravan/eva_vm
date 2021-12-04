/**
*   Eva Virtual Machine
*/

#ifndef __EvaVM_h
#define __EvaVM_h

#include <iostream>
#include <typeinfo>
#include <string>
#include <vector>

#include "../Logger.h"
#include "../bytecode/OpCode.h"

#define READ_BYTE() *ip++

class EvaVM {
    public:
        EvaVM() {}

    void exec(const std::string &program) {
        // 1. Parse to AST

        // 2. Compile to Bytecode
        code = {2};

        // Set IP to beginning
        ip = &code[0];

        return eval();
    }

    /**
     * Main Eval Loop
     */
    void eval() {
        for(;;) {
            int opcode = READ_BYTE();
            log(opcode);
            switch(opcode) {
                case OP_HALT:
                    return;
                default:
                    DIE << "Unknown opcode: " << std::hex << opcode;
            }
        }    
    }

    /**
     * Instruction Pointer
     */ 
    uint8_t* ip;

    /**
     *  Bytecode
     */
    std::vector<uint8_t> code;
};

#endif

