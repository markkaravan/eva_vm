#ifndef EvaValue_h
#define EvaValue_h

#include <string>

enum class EvaValueType {
    NUMBER,
    BOOLEAN,
    OBJECT,
};

enum class ObjectType {
    STRING,
    CODE,
    NATIVE,
    FUNCTION,
};

/**
 *  Base object
 */ 
struct Object {
    Object(ObjectType type) : type(type) {}
    ObjectType type;
};

/**
 *  String object
 */ 
struct StringObject : public Object {
    StringObject(const std::string& str)
        : Object(ObjectType::STRING), string(str) {}
    std::string string;
};

/**
 *  Native function
 */ 
using NativeFn = std::function<void()>;

struct NativeObject : public Object {
    NativeObject(NativeFn function, const std::string& name, size_t arity)
        : Object(ObjectType::NATIVE),
            function(function),
            name(name),
            arity(arity) {}

    /**
     *  Native function
     */ 
    NativeFn function;

    /**
     *  Function name
     */ 
    std::string name;

    /**
     *  Number of parameters
     */ 
    size_t arity;
};

// -------------------------------------------------------

/**
 * EvaValue (tagged union)
 */ 
struct EvaValue {
    EvaValueType type;
    union {
        double number;
        bool boolean;
        Object* object;
    };
};

struct LocalVar {
    std::string name;
    size_t scopeLevel;
};


struct CodeObject : public Object {
    CodeObject(const std::string& name, size_t arity) 
        : Object(ObjectType::CODE), name(name), arity(arity) {} 

    /**
     * Name of the unit (usually function name)
     */ 
    std::string  name;

    /**
     * Number of parameters
     */ 
    size_t arity;
    
    /**
     * Constant Pool
     */ 
    std::vector<EvaValue> constants;

    /**
     * Bytecode
     */ 
    std::vector<uint8_t> code;

    /**
     * Current scope level
     */ 
    size_t scopeLevel = 0;

    /**
     * Locals
     */ 
    std::vector<LocalVar> locals;

    /**
     * Cell var names -- free variables + own cells
     */ 
    std::vector<std::string> cellNames;

    /**
     * Free vars count
     */ 
    size_t freeCount = 0;

    /**
     * Adds a local within the current scope level
     */ 
    void addLocal(const std::string& name) {
        locals.push_back({name, scopeLevel});
        showLocals();
    }

    void showLocals() {
        for (auto it = locals.begin(); it != locals.end(); ++it) {
            std::cout << it->name << ":" << it->scopeLevel << std::endl;
        }
    }

    /**
     * Adds a constant
     */ 
    void addConst(const EvaValue& value) { constants.push_back(value); }

    /**
     * Get local index
     */
    int getLocalIndex(const std::string& name) {
        if (locals.size() > 0) { 
            for (auto i = (int)locals.size() - 1; i >= 0; i--) {
                if (locals[i].name == name) {
                    return i;
                }
            }
        }
        return -1;
    } 

    /**
     * Get cell index
     */
    int getCellIndex(const std::string& name) {
        if (locals.size() > 0) { 
            for (auto i = (int)cellNames.size() - 1; i >= 0; i--) {
                if (cellNames[i] == name) {
                    return i;
                }
            }
        }
        return -1;
    } 
};

// -------------------------------------------------------

/**
 * Function object
 */ 
struct FunctionObject: public Object {
    FunctionObject(CodeObject* co) : Object(ObjectType::FUNCTION), co(co) {}
    /**
     * Reference to the code object;
     * contains function code, locals, etc.
     */
    CodeObject* co;
};

/**
 * Constructors
 */
#define NUMBER(value) ((EvaValue){EvaValueType::NUMBER, .number = value})
#define BOOLEAN(value) ((EvaValue){EvaValueType::BOOLEAN, .boolean = value})

#define ALLOC_STRING(value)                 \
    ((EvaValue) {EvaValueType::OBJECT, .object = (Object*)new StringObject(value)})

#define ALLOC_CODE(name, arity)             \
    ((EvaValue) {EvaValueType::OBJECT,      \
            .object = (Object*)new CodeObject(name, arity)})

#define ALLOC_NATIVE(fn, name, arity)       \
    ((EvaValue) {EvaValueType::OBJECT,      \
            .object = (Object*)new NativeObject(fn, name, arity)})

#define ALLOC_FUNCTION(co)                  \
    ((EvaValue){EvaValueType::OBJECT,       \
            .object = (Object*)new FunctionObject(co)})

/**
 * Accessors
 */
#define AS_NUMBER(evaValue) ((double)(evaValue).number)
#define AS_BOOLEAN(evaValue) ((bool)(evaValue).boolean)
#define AS_OBJECT(evaValue) ((Object*)(evaValue).object)

#define AS_STRING(evaValue) ((StringObject*)(evaValue).object)
#define AS_CPPSTRING(evaValue) (AS_STRING(evaValue)->string)

#define AS_CODE(evaValue) ((CodeObject*)(evaValue).object)
#define AS_NATIVE(evaValue) ((NativeObject*)(evaValue).object)
#define AS_FUNCTION(evaValue) ((FunctionObject*)(evaValue).object)

/**
 * Testers
 */
#define IS_NUMBER(evaValue) ((evaValue).type == EvaValueType::NUMBER)
#define IS_BOOLEAN(evaValue) ((evaValue).type == EvaValueType::BOOLEAN)
#define IS_OBJECT(evaValue) ((evaValue).type == EvaValueType::OBJECT)

#define IS_OBJECT_TYPE(evaValue, objectType) \
    (IS_OBJECT(evaValue) && AS_OBJECT(evaValue)->type == objectType)

#define IS_STRING(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::STRING)
#define IS_CODE(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::CODE)
#define IS_NATIVE(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::NATIVE)
#define IS_FUNCTION(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::FUNCTION)


/**
 * String representation used in constants for debugging
 */
std::string evaValueToTypeString(const EvaValue &evaValue) {
    if (IS_NUMBER(evaValue)) {
        return "NUMBER";
    } else if (IS_BOOLEAN(evaValue)) {
        return "BOOLEAN";
    } else if (IS_STRING(evaValue)) {
        return "STRING";
    } else if (IS_CODE(evaValue)) {
        return "CODE";
    } else if (IS_NATIVE(evaValue)) {
        return "NATIVE";
    } else if (IS_FUNCTION(evaValue)) {
        return "FUNCTION";
    } else {
        DIE << "evaValueToTypeString: unknown type " << (int)evaValue.type;
    }
    return "";  // Unreachable
}

std::string evaValueToConstantString(const EvaValue &evaValue) {
    std::stringstream ss;
    if (IS_NUMBER(evaValue)) {
        ss << evaValue.number;
    } else if (IS_BOOLEAN(evaValue)) {
        ss << (evaValue.boolean == true ? "true" : "false");
    } else if (IS_STRING(evaValue)) {
        ss << '"' << AS_CPPSTRING(evaValue) << '"';
    } else if (IS_CODE(evaValue)) {
        auto code = AS_CODE(evaValue);
        ss << "code " << code << ": " << code->name << "/" << code->arity; 
    } else if (IS_FUNCTION(evaValue)) {
        auto fn = AS_FUNCTION(evaValue);
        ss << fn->co->name << "/" << fn->co->arity;
    } else if (IS_NATIVE(evaValue)) {
        auto fn = AS_NATIVE(evaValue);
        ss << fn->name << "/" << fn->arity;
    } else {
        DIE << "evaValueToConstantString: unknown type " << (int)evaValue.type;
    }
    return ss.str();
}

/**
 * Output Stream
 */ 
std::ostream& operator<<(std::ostream &os, const EvaValue &evaValue) {
    return os << "EvaValue (" << evaValueToTypeString(evaValue)
            << "): " << evaValueToConstantString(evaValue);
}

#endif