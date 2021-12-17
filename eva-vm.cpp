#include <iostream>

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


int main(int argc, char const *argv[]) {

    runTheTests();

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