#include <iostream>

#include "src/Logger.h"
#include "src/vm/EvaVM.h"
#include "src/vm/EvaValue.h"

int main(int argc, char const *argv[]) {
    EvaVM vm;

    auto result = vm.exec(R"(

        (var i 10)
        (var count 0)
        (while (> i 0)
            (begin
                (set i (- i 1))
                (set count (+ count 1))))
        count
    )");

    std::cout << "\n";
    log(result);

    std::cout << "All done!\n";

    return 0;
}