#include <iostream>
#include <vector>

#include "src/Logger.h"
#include "src/vm/EvaVM.h"
#include "src/vm/EvaValue.h"

struct TestCase {
    EvaValue expectedResult;
    const char* testProgram;
    // bool showDisassembler;
    bool showStackDump;
};

struct TestResult {
    EvaValue expectedResult;
    EvaValue actualResult;
    const char* testProgram;
    bool passed;
};

TestResult runTest(EvaValue expectedResult, const char* testProgram, bool showStackDump) {
    EvaVM vm;

    std::cout << std::endl << std::endl << "======================" << std::endl
        << "Testing this program: " << std::endl
        << testProgram << std::endl
        << "======================" << std::endl << std::endl;

    auto actualResult = vm.exec(testProgram);

    // Let's see if the test passed
    bool passed = false;
    if (IS_NUMBER(expectedResult)) {
        passed = AS_NUMBER(actualResult) == AS_NUMBER(expectedResult);
    } else if (IS_STRING(expectedResult)) {
        passed = AS_CPPSTRING(actualResult) == AS_CPPSTRING(expectedResult);
    }

    if (passed) {
        std::cout << "-- Test passed --" << std::endl;
    } else {
        std::cout << "-- Test failed --" << std::endl;
    }

    if (showStackDump == true) {
        std::cout << "====================" << std::endl << " This is the final stack: " << std::endl;
        vm.dumpStack(0);
    }

    std::cout << "\n";
    if (!passed) {
        log(actualResult);
    }

    std::cout << "All done!\n";
    return TestResult {
        expectedResult,
        actualResult,
        testProgram,
        passed
    };
}

void runTheTests () {
    std::vector<TestResult> results;

    results.push_back(runTest(NUMBER(4), R"(
        4
    )", false));

    results.push_back(runTest(NUMBER(5), R"(
        (+ 2 3)
    )", false));

    results.push_back(runTest(NUMBER(14), R"(
        (+ 2 (* 3 4))
    )", false));

    results.push_back(runTest(ALLOC_STRING("foo"), R"(
        "foo"
    )", false));

    results.push_back(runTest(ALLOC_STRING("foo"), R"(
        (var x "foo")
        x
    )", false));

    results.push_back(runTest(ALLOC_STRING("bar"), R"(
        (begin 
            "foo"
            "bar")
    )", false));

    results.push_back(runTest(NUMBER(7), R"(
        (begin 
            "foo"
            (+ 3 4))
    )", false));

    results.push_back(runTest(NUMBER(30), R"(
        (begin 
            (var a 10)
            (var b 20)
            (+ a b))
    )", false));

    results.push_back(runTest(NUMBER(30), R"(
        (var x 1)
        (begin 
            (var a 10)
            (var b 20)
            (+ a b))
    )", false));

    results.push_back(runTest(NUMBER(7), R"(
        (var x 1)
        (var y 2)
        (var z 3)
        (+ x (* y z))
    )", false));

    results.push_back(runTest(NUMBER(7), R"(
        (begin
            (var x 1)
            (var y 2)
            (var z 3)
            (+ x (* y z)))
    )", false));

    results.push_back(runTest(NUMBER(2), R"(
        (begin
            (begin
                (var x 1)
                (var y 2)
                (var z 3)
                (+ x (* y z)))
            (+ 1 1)   
            )
    )", false));


    results.push_back(runTest(NUMBER(1), R"(
        (var x 1)
        x
    )", false));

    results.push_back(runTest(NUMBER(3), R"(
        (var x 1)
        (var y 2)
        (+ x y)
    )", false));

    results.push_back(runTest(NUMBER(9), R"(
        (def square (x) (* x x))
        (square 3)
    )", false));

    results.push_back(runTest(NUMBER(15), R"(
        (begin
            (var q 15)
            q)
    )", false));

    results.push_back(runTest(NUMBER(15), R"(
        (begin
            (begin
                (var q 15)
                q))
    )", false));

    results.push_back(runTest(NUMBER(15), R"(
        (begin
            (begin
                (var b 20)
                (var q 15)
                q))
    )", false));

    results.push_back(runTest(NUMBER(4), R"(
        (def square (x) (* x x))
        (square 2)
    )", false));

    results.push_back(runTest(NUMBER(9), R"(
        (def square (x) (* x x))
        (square 3)
    )", false));

    // This one yields very inconsistent segfaults wtf
    results.push_back(runTest(NUMBER(15), R"(
        (var x 1)
        (begin 
            (var a 10)
            (var b 20)
            (begin
                (var q 15)
                q
            ))
    )", false));

    results.push_back(runTest(NUMBER(25), R"(
        (var x 1)
        (begin 
            (var a 10)
            (var b 20)
            (begin
                (var q 15)
                (+ a q)
            ))
    )", false));

    results.push_back(runTest(NUMBER(16), R"(
        (var square (lambda (x) (* x x)))
        (square 4)
    )", false));

    results.push_back(runTest(NUMBER(5), R"(
        (var i 4)
        (set i 5)
        i
    )", false));


    results.push_back(runTest(NUMBER(6), R"(
        (def square (x) (* x x))
    )", false));


    // This one doesn't work.
    // Declarations are automatically popped from the stack
    // in OP_SET_GLOBAL in the vm
    // this happened in Lecture 20: Bytecode optimizations 
    // results.push_back(runTest(NUMBER(16), R"(
    //     (def square (x) (* x x))
    // )", false));

    // results.push_back(runTest(NUMBER(5), R"(
    //     (var i 0)
    //     (var total 0)
    //     (while 
    //         (< i 10)
    //         (begin 
    //             (set i (+ i 1))
    //             (set total (+ total 1))))
    //     total
    // )", false));


    std::cout << "=============================" << std::endl
        << "Results:" << std::endl;

    for (auto i = 0; i<results.size(); i++) {
        if (results[i].passed == true) {
            std::cout << "Passed" << std::endl;
        } else {
            std::cout << "Failed!" << std::endl 
                << results[i].testProgram << std::endl
                << "expected: " << results[i].expectedResult << std::endl
                << "actual: " << results[i].actualResult << std::endl;
        }

    }
}


void singleTest () {
    EvaVM vm;

    const char* source = R"(
        // (def square (x) (* x x))
    )";

    std::cout << "Running this code: " <<std::endl << source << std::endl;
    auto result = vm.exec(source);

    std::cout << "====================" << std::endl << " This is the final stack: " << std::endl;
    vm.dumpStack(0);
    log(result);

    std::cout << "All done" << std::endl;
}


int main(int argc, char const *argv[]) {

    runTheTests();

    //singleTest();

    return 0;
};

























        // (def square (x) (* x x))
        

        // (native-square 4)

        // (var square (lambda (x) (* x x)))
        // (square 4)

        // (begin
        //     (var a 10)
        //     (var b 20)
        //     (+ a b))

        // (def sum10 (a b)
        //     (begin
        //         (var x 10)
        //         (+ x (+ a b))))
        // (sum10 3 4)

        // (begin
        //     (var a 10)
        //     (var b 20)
        //     (+ a b))

        // (native-square 2)

        // (var x 10)

        // (def foo () x)

        // (begin
        //     (var y 100)
        //     (var q 300)
        //     q
        //     (+ y x)
        //     (begin
        //         (var z 200)
        //         z
        //         (def bar () (+ y z))
        //         (bar)))
        ///////////////////////////////


        // (var x 10)
        // (def foo () x)
        // (foo)

        // (begin
        //     (var v 100)
        //     (def bar () y)
        //     (bar))

        // (def factorial (x)
        //     (if (== x 1) 
        //         1 
        //         (* x (factorial (- x 1)))))
        // (factorial 3)

        // // ALso while loop