#include "src/vm/EvaVM.h"

int main(int argc, char const *argv[]) {
    EvaVM vm;

    vm.exec(R"(

        42

    )");

    return 0;
}