#include <iostream>
#include "src/Logger.h"
#include "src/vm/EvaVM.h"
#include "src/vm/EvaValue.h"

int main(int argc, char const *argv[]) {
    EvaVM vm;

    auto result = vm.exec(R"(

        (if (> 5 10) 1 2)

    )");

    std::cout << "\n";
    log(result);

    std::cout << "All done!\n";

    return 0;
}