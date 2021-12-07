#ifndef EvaCompiler_h
#define EvaCompiler_h

#include <map>
#include <string>

#include "../parser/EvaParser.h"
#include "../vm/EvaValue.h"
#include "../bytecode/OpCode.h"
#include "../disassembler/EvaDisassembler.h"
#include "../vm/Global.h"

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
        EvaCompiler(std::shared_ptr<Global> global) 
            : global(global),
              disassembler(std::make_unique<EvaDisassembler>(global)) {}

        // TODO: replace w/unique pointer
        // EvaDisassembler* disassembler = new EvaDisassembler();



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
                        // Variables
                        auto varName = exp.string;

                        // 1. Local vars:

                        auto localIndex = co->getLocalIndex(varName);

                        if (localIndex != -1) {
                            emit(OP_GET_LOCAL);
                            emit(localIndex);
                        }

                        // 2. Global vars:
                        else {
                            if (!global->exists(varName)) {
                                DIE << "[EvaCompiler]: Reference error: " << varName;
                            }

                            emit(OP_GET_GLOBAL);
                            emit(global->getGlobalIndex(varName));
                        }
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

                        //----------------------------------
                        // While loop:
                        /**
                         *  (while <test> <body>)
                         */ 
                        else if (op == "while") {
                            auto loopStartAddr = getOffset();

                            // Emit <test>:
                            gen(exp.list[1]);

                            // Lop end.  Init with 0 address, will be patched
                            emit(OP_JMP_IF_FALSE);
                            // NOTE: we use 2-byte addresses:
                            emit(0);
                            emit(0);
                            auto loopEndJmpAddr = getOffset() - 2;

                            // Emit <body>
                            gen(exp.list[2]);

                            // Goto loop start:
                            emit(OP_JMP);
                            emit(0);
                            emit(0);
                            patchJumpAddress(getOffset() - 2, loopStartAddr);

                            // Patch the end
                            auto loopEndAddr =  getOffset() + 1;
                            patchJumpAddress(loopEndJmpAddr, loopEndAddr);

                        }


                        //----------------------------------
                        // Variable declaration:

                        else if (op == "var") {
                            auto varName = exp.list[1].string;

                            // Initializer
                            gen(exp.list[2]);

                            // 1. Global vars:
                            if (isGlobalScope()) {
                                global->define(varName);
                                emit(OP_SET_GLOBAL);
                                emit(global->getGlobalIndex(varName));
                            }

                            // 2. Local vars: TODO
                            else {
                                co->addLocal(varName);
                                emit(OP_SET_LOCAL);
                                emit(co->getLocalIndex(varName));
                            }
                        }

                        else if (op == "set") {
                            auto varName = exp.list[1].string;

                            // Initialization
                            gen(exp.list[2]);

                            // 1. Local vars:
                            auto localIndex = co->getLocalIndex(varName);

                            if (localIndex != -1) {
                                emit(OP_SET_LOCAL);
                                emit(localIndex);
                            }

                            // 2. Global vars:  
                            else {
                                auto globalIndex = global->getGlobalIndex(varName);
                                if (globalIndex == -1) {
                                    DIE << "Reference error: " << varName << " is not defined.";
                                }
                                emit(OP_SET_GLOBAL);
                                emit(global->getGlobalIndex(varName));
                            }
                        }

                        //----------------------------------
                        // Blocks:
                        else if (op == "begin") {
                            scopeEnter(); 

                            // Compile each expression within the block:
                            for (auto i = 1; i<exp.list.size(); i++) {
                                // The value of the last expression is kept
                                // on the stack as the final result
                                bool isLast = i == exp.list.size() - 1;

                                auto isLocalDeclaration =
                                    isDeclaration(exp.list[i]) && !isGlobalScope();

                                // Generate expression code;
                                gen(exp.list[i]);

                                if (!isLast && !isLocalDeclaration) {
                                    emit(OP_POP);
                                }
                            }
                            scopeExit();
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

        /**
         * Global object
         */
        std::shared_ptr<Global> global;

        /**
         * Disassembler.
         */
        std::unique_ptr<EvaDisassembler> disassembler;


        /**
         * Enters new scope.
         */
        void scopeEnter() { co->scopeLevel++; }

        /**
         * Exits scope.
         */
        void scopeExit() { 
            auto varsCount = getVarsCountOnScopeExit();
            if (varsCount > 0) {
                emit(OP_SCOPE_EXIT);
                emit(varsCount);
            }

            co->scopeLevel--; 
        }

        /**
         * Determine scope.
         */        
        bool isGlobalScope() {
            return co->scopeLevel == 1;
        }

        bool isDeclaration(const Exp& exp) { return isVarDeclaration(exp); }

        bool isVarDeclaration(const Exp& exp) { return isTaggedList(exp, "var"); }

        bool isTaggedList(const Exp& exp, const std::string& tag) {
            return exp.type == ExpType::LIST && exp.list[0].type == ExpType::SYMBOL &&
                    exp.list[0].string == tag;
        }

        size_t getVarsCountOnScopeExit() {
            auto varsCount = 0;

            if (co->locals.size() > 0) {
                while (co->locals.back().scopeLevel ==  co->scopeLevel) {
                    co->locals.pop_back();
                    varsCount++;
                }
            }
            return varsCount;
        }

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