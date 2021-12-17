#include <iostream>
#include <string>
#include <fstream>

#include "src/Logger.h"
#include "src/vm/EvaVM.h"
#include "src/tests/Tests.h"


void singleTest (bool showDisassembler, bool showStacks) {
    EvaVM vm;

    const char* source = R"(
        (var x 10)
        (def foo () x)
        (begin 
            (var y 100) 
            (var q 7) 
            q           
            (+ y x)     
            (begin
                (var z 200)
                z      
                (def bar () (+ y z))
                (bar)))
    )";

    std::cout << "Running this code: " <<std::endl << source << std::endl;
    auto result = vm.exec(source, showDisassembler, showStacks);

    std::cout << "====================" << std::endl << " This is the final stack: " << std::endl;
    vm.dumpStack(0);
    log(result);

    std::cout << "All done" << std::endl;
}

void commandLine(int argc, char const *argv[]) {
    if (argc != 3) {
        std::cout << "\nUsage: eva-vm [options] \n\n"
                << "Options: \n"
                << "  -e, --expression 'Expression to parse'\n"
                << "  -f, --file       File to parse\n\n";
        return;
    }

    // Expression mode
    std::string mode = argv[1];

    // Program declaration
    std::string program;

    // Simple expression
    if (mode == "-e") {
        program = argv[2];
    }

    // Eva file
    else if (mode == "-f") {
        // Read the file
        std::ifstream programFile(argv[2]);
        std::stringstream buffer;
        buffer << programFile.rdbuf() << "\n";

        // Program
        program = buffer.str();
    }

    // VM instance
    EvaVM vm;


    bool showDisassembler = true;
    bool showStacks = false;
    auto result = vm.exec(program, showDisassembler, showStacks);
    log(result);
}

int main(int argc, char const *argv[]) {

    /**
     * Uncomment this function for production
     */
    commandLine(argc, argv);

    /**
     * Uncomment this function for TDD while developing
     */
    // runTheTests();

    /**
     * Uncomment this function for single test while developing
     */
    // bool showDisassembler = true;
    // bool showStacks = false;
    // singleTest(showDisassembler, showStacks);

    return 0;
};

























        // (def square (x) (* x x))
        

        // (native-square 4)

        // (var square (lambda (x) (* x x)))
        // (square 4)

        // (begin
        //     (var a 10)
        //     (var b 20)
        //     (+ a b))

        // (def sum10 (a b)
        //     (begin
        //         (var x 10)
        //         (+ x (+ a b))))
        // (sum10 3 4)

        // (begin
        //     (var a 10)
        //     (var b 20)
        //     (+ a b))

        // (native-square 2)

        // (var x 10)

        // (def foo () x)

        // (begin
        //     (var y 100)
        //     (var q 300)
        //     q
        //     (+ y x)
        //     (begin
        //         (var z 200)
        //         z
        //         (def bar () (+ y z))
        //         (bar)))
        ///////////////////////////////


        // (var x 10)
        // (def foo () x)
        // (foo)

        // (begin
        //     (var v 100)
        //     (def bar () y)
        //     (bar))

        // (def factorial (x)
        //     (if (== x 1) 
        //         1 
        //         (* x (factorial (- x 1)))))
        // (factorial 3)

        // // ALso while loop