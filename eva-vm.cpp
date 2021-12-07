#include <iostream>

#include "src/Logger.h"
#include "src/vm/EvaVM.h"
#include "src/vm/EvaValue.h"

int main(int argc, char const *argv[]) {
    EvaVM vm;

    auto result = vm.exec(R"(

        (var x 5)
        (set x (+ x 10))
        x

        (begin
            (var x 100)
            (begin
                (var x 200)
                x)
            x)
        x
    )");

    std::cout << "\n";
    log(result);

    std::cout << "All done!\n";

    return 0;
}