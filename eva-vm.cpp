#include <iostream>

#include "src/Logger.h"
#include "src/vm/EvaVM.h"
#include "src/vm/EvaValue.h"

int main(int argc, char const *argv[]) {
    EvaVM vm;

    auto result = vm.exec(R"(

        (var x 10)

        (native-square x)

    )");

    vm.dumpStack();

    std::cout << "\n";
    log(result);

    std::cout << "All done!\n";

    return 0;
}