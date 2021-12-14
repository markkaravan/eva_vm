#ifndef EvaCompiler_h
#define EvaCompiler_h

#include <map>
#include <string>

#include "../parser/EvaParser.h"
#include "../vm/EvaValue.h"
#include "../bytecode/OpCode.h"
#include "../disassembler/EvaDisassembler.h"
#include "../vm/Global.h"
#include "Scope.h"


//------------------------------------------------------------

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

#define FUNCTION_CALL(exp)                              \
    do {                                                \
        gen(exp.list[0]);                               \
        for (auto i=1; i<exp.list.size(); i++) {        \
            gen(exp.list[i]);                           \
        }                                               \
        emit(OP_CALL);                                  \
        emit(exp.list.size() - 1);                      \
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

            // Scope analysis
            analyze(exp, nullptr);

            // Generate recursively from top
            gen(exp);

            // Explicit Halt market
            emit(OP_HALT);
        }

        /**
         *  Scope analysis
         */
        void analyze(const Exp& exp, std::shared_ptr<Scope> scope) {
            /**
             * -----------------------------------------
             * Symbols
             */
            if (exp.type == ExpType::SYMBOL) {
                /**
                 * Boolean
                 */
                if (exp.string == "true" || exp.string == "false") {
                    // Do nothing
                } else {
                    scope->maybePromote(exp.string);
                }
            }


            /**
             * -----------------------------------------
             * Lists
             */
            else if (exp.type == ExpType::LIST) {
                auto tag = exp.list[0];

                /**
                 * -----------------------------------------
                 * Special cases
                 */
                if (tag.type == ExpType::SYMBOL) {
                    auto op = tag.string;

                    // Block scope:
                    if (op == "begin") {
                        auto newScope = std::make_shared<Scope>(
                            scope == nullptr ? ScopeType::GLOBAL : ScopeType::BLOCK, scope);

                        scopeInfo_[&exp] = newScope;
                        for (auto i = 1; i < exp.list.size(); ++i) {
                            analyze(exp.list[i], newScope);
                        }
                    }

                    // -------------------------------------
                    // Variable declaration:
                    else if (op == "var") {
                        scope->addLocal(exp.list[1].string);
                        analyze(exp.list[2], scope);
                    }

                    // -------------------------------------
                    // Functional declaration:
                    else if (op == "def") {
                        auto fnName = exp.list[1].string;

                        scope->addLocal(fnName);

                        auto newScope = std::make_shared<Scope>(ScopeType::FUNCTION, scope);
                        scopeInfo_[&exp] = newScope;

                        newScope->addLocal(fnName);

                        auto arity = exp.list[2].list.size();

                        // Params
                        for (auto i = 0; i<arity; i++) {
                            newScope->addLocal(exp.list[2].list[i].string);
                        }

                        // Body
                        analyze(exp.list[3], newScope);
                    }

                    else if (op == "lambda") {
                        auto newScope = std::make_shared<Scope>(ScopeType::FUNCTION, scope);
                        scopeInfo_[&exp] = newScope;

                        auto arity = exp.list[1].list.size();

                        // Params
                        for (auto i=0; i<arity; i++) {
                            newScope->addLocal(exp.list[1].list[i].string);
                        }

                        // Body
                        analyze(exp.list[2], newScope);
                    }

                    else {
                        for (auto i=1; i<exp.list.size(); ++i) {
                            analyze(exp.list[i], scope);
                        }
                    }
                // if (tag.type == ExpType::SYMBOL) {
                } else {
                    for (auto i = 1; i<exp.list.size(); ++i) {
                        analyze(exp.list[i], scope);
                    }
                }
            }
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

                        auto opCodeGetter = scopeStack_.top()->getNameGetter(varName);
                        emit(opCodeGetter);

                        // 1. Local vars:
                        if (opCodeGetter == OP_GET_LOCAL) {
                            emit(co->getLocalIndex(varName));
                        }

                        // 2. Global vars:
                        else if (opCodeGetter == OP_GET_CELL) {
                            emit(co->getCellIndex(varName));
                        }

                        // 3. Global vars:
                        else {
                            if (!global->exists(varName)) {
                                DIE << "[EvaCompiler]: Reference error: " << varName;
                            }
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

                            auto opCodeSetter = scopeStack_.top()->getNameSetter(varName);                            

                            // Special treatment of (var foo (lambda ...))
                            // to capture function name from variable:
                            if (isLambda(exp.list[2])) {
                                compileFunction(
                                    /* exp */       exp.list[2],
                                    /* name */      varName,
                                    /* params */    exp.list[2].list[1],
                                    /* body */      exp.list[2].list[2]);
                            } else {
                                gen(exp.list[2]);
                            }

                            // 1. Global vars:
                            if (opCodeSetter == OP_SET_GLOBAL) {
                                global->define(varName);
                                emit(OP_SET_GLOBAL);
                                emit(global->getGlobalIndex(varName));
                                // emit(OP_FAKE_TEST); // TODO remove
                                // TODO TESTING, WILL REMOVE
                                emit(OP_POP);
                            }
                            // 2. Cells:
                            else if (opCodeSetter == OP_SET_CELL) {
                                co->cellNames.push_back(varName);
                                emit(OP_SET_CELL);
                                emit(co->cellNames.size()-1);
                                // Explicitly po the value from the stack,
                                // since it's promoted to the heap:
                                emit(OP_POP);
                            }
                            // 3. Local vars:
                            else {
                                co->addLocal(varName);
                                // NOTE: no neet to explicitly "set" the var value, since the
                                // initializer is already on the stack at the needed slot
                            }
                        }

                        else if (op == "set") {
                            auto varName = exp.list[1].string;

                            auto opCodeSetter = scopeStack_.top()->getNameSetter(varName);

                            // Value:
                            gen(exp.list[2]);

                            // 1. Local vars:
                            if (opCodeSetter == OP_SET_LOCAL) {
                                emit(OP_SET_LOCAL);
                                emit(co->getLocalIndex(varName));
                            }

                            // 1. Local vars:
                            else if (opCodeSetter == OP_SET_CELL) {
                                emit(OP_SET_CELL);
                                emit(co->getCellIndex(varName));
                            }

                            // 3. Global vars:  
                            else {
                                auto globalIndex = global->getGlobalIndex(varName);
                                if (globalIndex == -1) {
                                    DIE << "Reference error: " << varName << " is not defined.";
                                }
                                emit(OP_SET_GLOBAL);
                                emit(globalIndex);
                                // TODO TESTING, WILL REMOVE
                                emit(OP_POP);
                            }
                        }

                        //----------------------------------
                        // Blocks:
                        else if (op == "begin") {
                            scopeStack_.push(scopeInfo_.at(&exp));
                            blockEnter(); 

                            // Compile each expression within the block:
                            for (auto i = 1; i<exp.list.size(); i++) {
                                // The value of the last expression is kept
                                // on the stack as the final result
                                bool isLast = i == exp.list.size() - 1;

                                // Local variable or function (should NON pop);
                                auto isDecl = isDeclaration(exp.list[i]);

                                // Generate expression code;
                                gen(exp.list[i]);

                                // TODO this is a monkey patch
                                if (isLast && isDecl) {
                                    co->code.pop_back();
                                }
                            }
                            blockExit();
                            scopeStack_.pop();
                        }
                        //----------------------------------
                        // Function declaration (def <name> <params> <body>)
                        //
                        // Sugar for: (var <name> (lambda <params> <body>))

                        else if (op == "def") {
                            auto fnName = exp.list[1].string;
                            compileFunction(
                                /* exp */       exp,
                                /* name */      fnName,
                                /* params */    exp.list[2],
                                /* body */      exp.list[3]);

                            // Define the function as a variable in our co:
                            if (isGlobalScope()) {
                                global->define(fnName);
                                emit(OP_SET_GLOBAL);
                                emit(global->getGlobalIndex(fnName));
                                // TODO TESTING, WILL REMOVE
                                emit(OP_POP);
                            } else {
                                co->addLocal(fnName);
                                // No need to set explicit "set" the var value
                                // since function is already on the stack at needed slot
                            }
                        }

                        //----------------------------------
                        // Lambda expression:
                        //
                        // (lambda (a b) (+ a b))
                        else if (op == "lambda") {
                            compileFunction(
                                /* exp */       exp,
                                /* name */      "lambda",
                                /* params */    exp.list[1],
                                /* body */      exp.list[2]);
                        }

                        //----------------------------------
                        // Function calls:
                        else {
                            // Named function calls
                            FUNCTION_CALL(exp);
                        }
                    }
                    //----------------------------------
                    // Lambda function calls:
                    //
                    // ((lambda (x) (* x x)) 2)

                    //if (tag.type == ExpType::SYMBOL) {
                    else {
                        // Inline lambda call.
                        FUNCTION_CALL(exp);
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
         * Compiles a function
         */
        void compileFunction(
            const Exp &exp, 
            const std::string fnName, 
            const Exp &params, 
            const Exp &body) {

            auto scopeInfo = scopeInfo_.at(&exp);
            scopeStack_.push(scopeInfo);
            
            auto arity = params.list.size();

            // Save previous code object
            auto prevCo = co;

            // Function code object:
            auto coValue = createCodeObjectValue(fnName, arity);
            co = AS_CODE(coValue);

            // Put 'free' and 'cells' from the scope into the
            // cellNames of the code object.
            co->freeCount = scopeInfo->free.size();
            co->cellNames.reserve(scopeInfo->free.size() + scopeInfo->cells.size());
            co->cellNames.insert(co->cellNames.end(), scopeInfo->free.begin(), 
                scopeInfo->free.end());
            co->cellNames.insert(co->cellNames.end(), scopeInfo->cells.begin(), 
                scopeInfo->cells.end());


            // Store new co as a constant:
            prevCo->constants.push_back(coValue);

            // Function name is registered as a local,
            // so the function can call itself recursively.
            co->addLocal(fnName);

            // Parameters are added as variables.
            for (auto i = 0; i < arity; i++) {
                auto argName = params.list[i].string;
                co->addLocal(argName);
                // NOTE: if the param is captured by cell, emit the code
                // for it.  We also don't pop the param value in this
                // case, since OP_SCOPE_EXIT would pop it.
                auto cellIndex = co->getCellIndex(argName);
                if (cellIndex != -1) {
                    emit(OP_SET_CELL);
                    emit(cellIndex);
                }
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

            scopeStack_.pop();
        }

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
         * Enters new block.
         */
        void blockEnter() { co->scopeLevel++; }

        /**
         * Exits block.
         */
        void blockExit() { 
            // Pop vars from the stack if they were declared
            // within this specific scope
            // auto varsCount = getVarsCountOnScopeExit();
            auto varsCount = 0;
            if (co->locals.size() > 0) {
                // TODO sometimes this will continue to find stuff in co->locals that
                // hasn't been named
                while (co->locals.back().name == "" 
                    && co->locals.back().scopeLevel ==  co->scopeLevel) {
                    co->locals.pop_back();
                    varsCount++;
                }
            }

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

        bool isDeclaration(const Exp& exp) { 
            return isVarDeclaration(exp) || 
                isFunctionDeclaration(exp) ||
                isLambda(exp)
            ; }

        bool isVarDeclaration(const Exp& exp) { return isTaggedList(exp, "var"); }

        bool isLambda(const Exp& exp) { return isTaggedList(exp, "lambda"); }

        // TODO in the notes, but not in the  code for some reason
        bool isFunctionDeclaration(const Exp& exp) { return isTaggedList(exp, "def"); }

        bool isBlock(const Exp& exp) { return isTaggedList(exp, "begin"); }

        bool isTaggedList(const Exp& exp, const std::string& tag) {
            return exp.type == ExpType::LIST 
                && exp.list[0].type == ExpType::SYMBOL 
                && exp.list[0].string == tag;
        }

        // size_t getVarsCountOnScopeExit() {
        //     auto varsCount = 0;

        //     if (co->locals.size() > 0) {
        //         while (co->locals.back().scopeLevel ==  co->scopeLevel) {
        //             co->locals.pop_back();
        //             varsCount++;
        //         }
        //     }
        //     return varsCount;
        // }

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
         *  Scope info
         */ 
        std::map<const Exp*, std::shared_ptr<Scope> > scopeInfo_;

        /**
         *  Scope stack
         */ 
        std::stack<std::shared_ptr<Scope>> scopeStack_;

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