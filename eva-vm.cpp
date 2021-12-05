#include <iostream>
#include "src/Logger.h"
#include "src/vm/EvaVM.h"

int main(int argc, char const *argv[]) {
    EvaVM vm;

    auto result = vm.exec(R"(

        (+ "hello " "world")

    )");

    log(result);

    std::cout << "All done!\n";

    return 0;
}