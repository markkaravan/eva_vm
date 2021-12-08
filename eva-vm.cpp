#include <iostream>

#include "src/Logger.h"
#include "src/vm/EvaVM.h"
#include "src/vm/EvaValue.h"


int main(int argc, char const *argv[]) {
    EvaVM vm;

    auto result = vm.exec(R"(

        (def square (x) (* x x))

        // (def sum (a b)
        //     (begin
        //         (var x 10)
        //         (+ x (+ a b))))

        // (begin
        //     (var a 10)
        //     (var b 20)
        //     (+ a b))

        // (def factorial (x)
        //     (if (== x 1) 
        //         1 
        //         (* x (factorial (- x 1)))))

        // (factorial 2)

        // (def square (x) (* x x))

        // (var square2 (lambda (x) (* x x)))

        // (native-square 2)

    )");

    vm.dumpStack();

    std::cout << "\n";
    log(result);

    std::cout << "All done!\n";

    return 0;
}