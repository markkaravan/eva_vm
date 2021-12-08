#include <iostream>

#include "src/Logger.h"
#include "src/vm/EvaVM.h"
#include "src/vm/EvaValue.h"

// TODO:
// The following code:

//         (def sum (a b)
//             (begin
//                 (var x 10)
//                 (+ x (+ a b))))

// produces this: 
// 0000           0101        CONST                1 (sum/2)
// 0002           1300        SET_LOCAL            0 (sum)
// 0004           1401        SCOPE_EXIT           1
// 0006           00          HALT
// Segmentation fault: 11

// should be: 
// 0000           0101        CONST                1 (sum/2)
// 0002           1001        SET_GLOBAL           0 (sum)
// 0004           00          HALT
// result = EvaValue (FUNCTION): sum/2


int main(int argc, char const *argv[]) {
    EvaVM vm;

    auto result = vm.exec(R"(

        // (def square (x) (* x x))

        // (def sum (a b)
        //     (begin
        //         (var x 10)
        //         (+ x (+ a b))))

        // (begin
        //     (+ 3 4)
        //     (* 4 5))
        (def factorial (x)
            (if (== x 1) 
                1 
                (* x (factorial (- x 1)))))

        (factorial 5)

    )");

    vm.dumpStack();

    std::cout << "\n";
    log(result);

    std::cout << "All done!\n";

    return 0;
}