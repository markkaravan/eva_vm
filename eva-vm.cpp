#include <iostream>
#include "src/Logger.h"
#include "src/vm/EvaVM.h"

int main(int argc, char const *argv[]) {
    EvaVM vm;

    auto result = vm.exec(R"(

        (+ "Hello" "World")

    )");

    log(AS_CPPSTRING(result));

    std::cout << "All done!\n";

    return 0;
}