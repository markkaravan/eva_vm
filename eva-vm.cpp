#include <iostream>

#include "src/Logger.h"
#include "src/vm/EvaVM.h"
#include "src/vm/EvaValue.h"


int main(int argc, char const *argv[]) {
    EvaVM vm;

    // TODO set up some kind of rudimentary TDD
    auto result = vm.exec(R"(

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

        (var x 10)

        (def foo () x)

        (begin
            (var y 100)
            (var q 300)
            q
            (+ y x)
            (begin
                (var z 200)
                z
                (def bar () (+ y z))
                (bar)))
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

    )");

    std::cout << "====================" << std::endl << " This is the final stack: " << std::endl;
    vm.dumpStack(0);

    std::cout << "\n";
    log(result);

    std::cout << "All done!\n";

    return 0;
}