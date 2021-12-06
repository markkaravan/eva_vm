#include <iostream>
#include "src/Logger.h"
#include "src/vm/EvaVM.h"

int main(int argc, char const *argv[]) {
    EvaVM vm;

    auto result = vm.exec(R"(

        (if (!= 5 5) 1 2)

    )");

    log(result);

    std::cout << "All done!\n";

    return 0;
}