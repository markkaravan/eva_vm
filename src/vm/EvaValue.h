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

#define NUMBER(value) ((EvaValue){EvaValueType::NUMBER, .number = value})

#endif