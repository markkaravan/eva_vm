# Eva Virtual Machine
Based on [Eva VM course](https://www.dmitrysoshnikov.education/p/virtual-machine) by [Dmitry Soshnikov](https://www.dmitrysoshnikov.education/).

## Compilation
```
clang++ -std=c++17 -Wall -ggdb3 ./eva-vm.cpp -o ./eva-vm
```

## Execution
### from command line:
```
./eva-vm -e '(+ "Hello, " "World")'
```
### from file:
```
./eva-vm -f test.eva
```