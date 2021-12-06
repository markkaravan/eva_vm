#ifndef __OpCode_h
#define __OpCode_h

/**
 *  Stops the program
 */ 
#define OP_HALT 0x00

/**
 *  Pushes a const onto the stack
 */
#define OP_CONST 0x01

/**
 *  Binary math instructions
 */
#define OP_ADD 0x02
#define OP_SUB 0x03
#define OP_MUL 0x04
#define OP_DIV 0x05

/**
 *  Comparison
 */
#define OP_COMPARE 0x06

/**
 *  Control flow: jump if the value on the stack is false.
 */
#define OP_JMP_IF_FALSE 0x07

/**
 *  Unconditional jump
 */
#define OP_JMP 0x08

#endif