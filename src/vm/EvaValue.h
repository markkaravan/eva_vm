#ifndef EvaValue_h
#define EvaValue_h

enum class EvaValueType {
    NUMBER
};

/**
 * EvaValue (tagged union)
 */ 
struct EvaValue {
    EvaValueType type;
    union {
        double number;
    };
};

/**
 * Constructors
 */
#define NUMBER(value) ((EvaValue){EvaValueType::NUMBER, .number = value})

/**
 * Accessors
 */
#define AS_NUMBER(evaValue) ((double)(evaValue).number)

#endif