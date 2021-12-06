#ifndef EvaCompiler_h
#define EvaCompiler_h

#include <map>
#include <string>

#include "../parser/EvaParser.h"
#include "../vm/EvaValue.h"
#include "../bytecode/OpCode.h"
#include "../disassembler/EvaDisassembler.h"

#define ALLOC_CONST(tester, converter, allocator, value)    \
    do {                                                    \
        for (auto i=0; i<co->constants.size(); i++) {       \
            if (!tester(co->constants[i])) {                \
                continue;                                   \
            }                                               \
            if (converter(co->constants[i]) == value) {     \
                return i;                                   \
            }                                               \
        }                                                   \
        co->constants.push_back(allocator(value));          \
    } while (false)

#define GEN_BINARY_OP(op)       \
    do {                        \
        gen(exp.list[1]);       \
        gen(exp.list[2]);       \
        emit(op);               \
    } while (false)

class EvaCompiler {
    public:
        EvaCompiler() 
            : disassembler(std::make_unique<EvaDisassembler>()) {}

        /**
         *  Main compile API
         */ 
        CodeObject* compile(const Exp& exp) {
            // Allocate new code object:
            co = AS_CODE(ALLOC_CODE("main"));

            // Generate recursively from top
            gen(exp);

            // Explicit Halt market
            emit(OP_HALT);

            return co;
        }

        /**
         *  Main compile loop
         */
        void gen(const Exp& exp) {
            switch (exp.type) {
                /**
                 *  Numbers
                 */ 
                case ExpType::NUMBER:
                    emit(OP_CONST);
                    emit(numericConstIdx(exp.number));
                    break;
                /**
                 *  Strings
                 */ 
                case ExpType::STRING:
                    emit(OP_CONST);
                    emit(stringConstIdx(exp.string));
                    break;
                /**
                 *  Symbols (variables, operators)
                 */ 
                case ExpType::SYMBOL:
                    // Booleans
                    if (exp.string == "true" || exp.string == "false") {
                        emit(OP_CONST);
                        emit(booleanConstIdx(exp.string == "true" ? true : false));
                    } else {
                        // TODO variables
                    }
                    break;
                /**
                 *  Lists (variables, operators)
                 */ 
                case ExpType::LIST:
                    auto tag = exp.list[0];
                    if (tag.type == ExpType::SYMBOL) {
                        auto op = tag.string;

                        //-----------------------------------
                        //  Binary math operations:
                        if (op == "+") {
                            GEN_BINARY_OP(OP_ADD);
                        }

                        else if (op == "-") {
                            GEN_BINARY_OP(OP_SUB);
                        }

                        else if (op == "*") {
                            GEN_BINARY_OP(OP_MUL);
                        }

                        else if (op == "/") {
                            GEN_BINARY_OP(OP_DIV);
                        }

                        //----------------------------------
                        // Compare operations: (> 5 10)
                        else if (compareOps_.count(op) != 0) {
                            gen(exp.list[1]);
                            gen(exp.list[2]);
                            emit(OP_COMPARE);
                            emit(compareOps_[op]);
                        }

                        //----------------------------------
                        // Branch instructions:
                        else if (op == "if") {
                            // Emit <test>:
                            gen(exp.list[1]);

                            // Else branch.  Init with a 0 address
                            emit(OP_JMP_IF_FALSE);
                            // NOTE: we use 2-byte addresses:
                            emit(0);
                            emit(0);
                            auto elseJmpAddr = getOffset() - 2;

                            // Emit <consequent>
                            gen(exp.list[2]);

                            emit(OP_JMP);
                            // NOTE: we use 2-byte addresses:
                            emit(0);
                            emit(0);
                            auto endAddr = getOffset() - 2;

                            // Patch the else branch address
                            auto elseBranchAddr = getOffset();
                            patchJumpAddress(elseJmpAddr, elseBranchAddr);

                            // Emit <alternate> if we have it
                            if (exp.list.size() == 4) {
                                gen(exp.list[3]);
                            }

                            auto endBranchAddr = getOffset();
                            patchJumpAddress(endAddr, endBranchAddr);
                        }
                    }
                    break;
            }
        }

    /**
     *  Disassemble all compliication units
     */ 
    void disassembleBytecode() { disassembler->disassemble(co); }

    private:
        std::unique_ptr<EvaDisassembler> disassembler;

        size_t getOffset() { return  co->code.size(); }

        /**
         *  Allocates a numeric constant
         */ 
        size_t numericConstIdx(double value) {
            ALLOC_CONST(IS_NUMBER, AS_NUMBER, NUMBER, value);
            return co->constants.size() - 1;
        }

        /**
         *  Allocates a string constant
         */ 
        size_t stringConstIdx(const std::string& value) {
            ALLOC_CONST(IS_STRING, AS_CPPSTRING, ALLOC_STRING, value);
            return co->constants.size() - 1;
        }

        /**
         *  Allocates a boolean constant
         */ 
        size_t booleanConstIdx(bool value) {
            ALLOC_CONST(IS_BOOLEAN, AS_BOOLEAN, BOOLEAN, value);
            return co->constants.size() - 1;
        }

        void emit(uint8_t code) { co->code.push_back(code); }

        /**
         * Write byte at offset
         */
        void writeByteAtOffset(size_t offset, uint16_t value) {
            co->code[offset] = value;
        }

        /**
         * Patches jump address
         */
        void patchJumpAddress(size_t offset, uint16_t value) {
            writeByteAtOffset(offset, (value >> 8) & 0xff);
            writeByteAtOffset(offset + 1, value & 0xff);
        }

    /**
     *  Compiling code object
     */ 
    CodeObject* co;

    /**
     *  Comparison map
     */ 
    static std::map<std::string, uint8_t> compareOps_;
};

std::map<std::string, uint8_t> EvaCompiler::compareOps_ = {
    {"<", 0}, {">", 1}, {"==", 2}, {">=", 3}, {"<=", 4}, {"!=", 5}, 
};

#endif