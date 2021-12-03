/**
*   Eva Virtual Machine
*/

#ifndef __EvaVM_h
#define __EvaVM_h

#include <string>
#include <vector>

#include "../bytecode/OpCode.h"

#define READ_BYTE() *ip++

class EvaVM {
    public:
        EvaVM() {}

    void exec(const std::string &program) {
        // 1. Parse to AST

        // 2. Compile to Bytecode
        code = {OP_HALT};

        // Set IP to beginning
        ip = &code[0];

        return eval();
    }

    /**
     * Main Eval Loop
     */
    void eval() {
        for(;;) {
            switch(READ_BYTE()) {
                case OP_HALT:
                    return;
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

