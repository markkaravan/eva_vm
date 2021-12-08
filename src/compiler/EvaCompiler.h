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
        co->addConst(allocator(value));          \
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
        void compile(const Exp& exp) {
            // Allocate new code object:
            co = AS_CODE(createCodeObjectValue("main"));
            main = AS_FUNCTION(ALLOC_FUNCTION(co));

            // Generate recursively from top
            gen(exp);

            // Explicit Halt market
            emit(OP_HALT);
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
                        // For loop:

                        // TODO: see end of while loop video


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
                        //----------------------------------
                        // Function declaration (def <name> <params> <body>)
                        else if (op == "def") {
                            auto fnName = exp.list[1].string;
                            auto params = exp.list[2].list;
                            auto arity = params.size();
                            auto body = exp.list[3];

                            // Save previous code object:
                            auto prevCo = co;

                            // Function code object:
                            auto coValue = createCodeObjectValue(fnName, arity);
                            co = AS_CODE(coValue);

                            // Store new co as a constant:
                            prevCo->constants.push_back(coValue);

                            // Function name is registered as a local,
                            // so the function can call itself recursively.
                            co->addLocal(fnName);

                            // Parameters are added as variables.
                            for (auto i = 0; i < arity; i++) {
                                auto argName = params[i].string;
                                co->addLocal(argName);
                            }

                            // Compile body in the new code object:
                            gen(body);

                            // If we don't have explicit block which pops locals,
                            // we should pop arguments (if any) - callee cleanup.
                            // +1 is for the function itself which is set as a local.
                            if (!isBlock(body)) {
                                emit(OP_SCOPE_EXIT);
                                emit(arity + 1);
                            }

                            // Explicit return to restore caller address.
                            emit(OP_RETURN);

                            // Create the function:
                            auto fn = ALLOC_FUNCTION(co);

                            // Restore the code object:
                            co = prevCo;

                            // Add function as a constant to our co:
                            co->addConst(fn);

                            // And emit code for this new constant:
                            emit(OP_CONST);
                            emit(co->constants.size() - 1);

                            //  TODO: segfault problem in eva-vm.cpp here
                            // Define the function as a variable in our co:
                            if (!isGlobalScope()) {
                                global->define(fnName);
                                emit(OP_SET_GLOBAL);
                                emit(global->getGlobalIndex(fnName));
                            } else {
                                co->addLocal(fnName);
                                emit(OP_SET_LOCAL);
                                emit(co->getLocalIndex(fnName));
                            }
                        }

                        //----------------------------------
                        // Function calls:
                        else {
                            // Push function onto the stack:
                            gen(exp.list[0]);

                            // Arguments:
                            for (auto i=1; i < exp.list.size(); i++) {
                                gen(exp.list[i]);
                            }

                            emit(OP_CALL);
                            emit(exp.list.size() - 1);
                        }
                    }
                    break;
            }
        }

        /**
         *  Disassemble all compliication units
         */ 
        void disassembleBytecode() { 
            for (auto& co_ : codeObjects_) {
                disassembler->disassemble(co_);
            }
        } 

        /**
         * Returns main function (entry point).
         */
        FunctionObject* getMainFunction() { return main; } 

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
         * Creates a new code object.
         */
        EvaValue createCodeObjectValue(const std::string &name, size_t arity = 0) {
            auto coValue = ALLOC_CODE(name, arity);
            auto co = AS_CODE(coValue);
            codeObjects_.push_back(co);
            return coValue;
        }

        /**
         * Enters new scope.
         */
        void scopeEnter() { co->scopeLevel++; }

        /**
         * Exits scope.
         */
        void scopeExit() { 
            // Pop vars from the satack if they were declared
            // within this specific scope
            auto varsCount = getVarsCountOnScopeExit();

            if (varsCount > 0 || co->arity > 0) {
                emit(OP_SCOPE_EXIT);

                // +1 for the function itself
                if (isFunctionBody()) {
                    varsCount += co->arity + 1;
                }

                emit(varsCount);
            }

            co->scopeLevel--; 
        }

        /**
         * Determine scope.
         */        
        bool isGlobalScope() { return co->name == "main" && co->scopeLevel == 1; }

        bool isFunctionBody() { return co->name != "main" && co->scopeLevel == 1; }

        bool isDeclaration(const Exp& exp) { return isVarDeclaration(exp); }

        bool isVarDeclaration(const Exp& exp) { return isTaggedList(exp, "var"); }

        bool isBlock(const Exp& exp) { return isTaggedList(exp, "begin"); }

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
         *  Main entry point for function
         */ 
        FunctionObject* main;

        /**
         *  All code objects
         */ 
        std::vector<CodeObject*> codeObjects_;

        /**
         *  Comparison map
         */ 
        static std::map<std::string, uint8_t> compareOps_;
};

std::map<std::string, uint8_t> EvaCompiler::compareOps_ = {
    {"<", 0}, {">", 1}, {"==", 2}, {">=", 3}, {"<=", 4}, {"!=", 5}, 
};

#endif