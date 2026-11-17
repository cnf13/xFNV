#pragma once

#include <stdint.h>

class Script;
class ScriptLocals;
class TESObjectREFR;

// Why evaluating an expression failed.
enum EXPRESSION_ERROR : int32_t {
    ERROR_EXPRESSION_NOERROR = 0,
    ERROR_EXPRESSION_OUTOFMEMORY = 1,
    ERROR_EXPRESSION_STACKOVERFLOW = 2,
    ERROR_EXPRESSION_STACKUNDERFLOW = 3,
    ERROR_EXPRESSION_DIVIDEBYZERO = 4,
    ERROR_EXPRESSION_SYNTAX = 5,
    ERROR_EXPRESSION_BADOBJECTPOINTER = 6,
    ERROR_EXPRESSION_COUNT = 7,
};

// The operators of a script expression, in the order of the operator table.
enum EXPRESSION_OPERATORS : int32_t {
    EXP_LEFTPAREN = 0,
    EXP_RIGHTPAREN = 1,
    EXP_AND = 2,
    EXP_OR = 3,
    EXP_LESSEQUAL = 4,
    EXP_LESS = 5,
    EXP_GREATEREQUAL = 6,
    EXP_GREATER = 7,
    EXP_EQUAL = 8,
    EXP_NOTEQUAL = 9,
    EXP_SUBTRACT = 10,
    EXP_ADD = 11,
    EXP_MULTIPLY = 12,
    EXP_DIVIDE = 13,
    EXP_MODULUS = 14,
    EXP_UNARYMINUS = 15,
    EXP_COUNT = 16,
};

// One operand on the evaluation stack: a long or a double, tagged by Type.
struct ExpressionOperand {
    char Type;

    union {
        int32_t eoLong;
        double eoDouble;
    };
};

// A fixed-capacity stack of N items; top indexes the top item (-1 when empty).
template <class T, int32_t N>
class Stack {
public:
    // An empty stack: top -1.
    Stack() : top(-1) {}

    // Each instantiation's members are defined per instance in Expression.cpp. push returns no item.
    void push(T c);
    T pop();
    int32_t empty();
    int32_t full();
    // Empties the stack and returns this.
    Stack* reset();

    T items[N];
    int32_t top;
};

template <class T, int32_t N>
void Stack<T, N>::push(T c) {
    items[++top] = c;
}

template <class T, int32_t N>
T Stack<T, N>::pop() {
    return items[top--];
}

template <class T, int32_t N>
int32_t Stack<T, N>::empty() {
    return top == -1;
}

template <class T, int32_t N>
int32_t Stack<T, N>::full() {
    return top + 1 == N;
}

template <class T, int32_t N>
Stack<T, N>* Stack<T, N>::reset() {
    top = -1;
    return this;
}

// Evaluates a script expression: converts the infix text to postfix and runs it on an operand stack.
class CExpression {
public:
    CExpression();
    // The destructor is empty.
    ~CExpression();

    uint32_t GetNextToken(
        char** appInput,
        char* apToken,
        EXPRESSION_OPERATORS& aeOperator,
        bool abEndian,
        const char** apScriptData
    );
    double Eval(
        const char* sptr,
        TESObjectREFR* apFunctionOwner,
        TESObjectREFR* apContainer,
        Script* script,
        ScriptLocals* scriptLocals,
        int32_t postfixSize,
        bool abEndian
    );

protected:
    // Records errorCode and logs its text.
    void SetError(EXPRESSION_ERROR errorCode);

public:
    char* InfixToPostfix(char* apInfix, uint32_t& auiSize);

    EXPRESSION_ERROR m_ExpressionError;
    char m_Infix[512];
    char m_Postfix[512];
    Stack<EXPRESSION_OPERATORS, 64> m_charStack;
    Stack<ExpressionOperand, 32> m_operandStack;
};

// One entry of the operator table: the operator, its precedence and its text.
struct SCRIPT_OPERATOR {
    EXPRESSION_OPERATORS eCode;
    char cPrecedence;
    char pOperator[3];
};

using ExpressionErrorTextArray = const char* [ERROR_EXPRESSION_COUNT];
inline ExpressionErrorTextArray expressionErrors{
    "Expression Error: NOERROR",         "Expression Error: OUTOFMEMORY",  "Expression Error: STACKOVERFLOW",
    "Expression Error: STACKUNDERFLOW",  "Expression Error: DIVIDEBYZERO", "Expression Error: SYNTAX",
    "Expression Error: BADOBJECTPOINTER"
};

// The texts of SCRIPT_ERROR (TESScriptCompiler.h), one per value: 18 pointers up to
// scriptOperators.
using ScriptErrorTextArray = const char* [18];
inline ScriptErrorTextArray kScriptErrors{
    "Script Error: NOERROR",         "Script Error: SCRIPT_NAME_MISSING", "Script Error: SYNTAX",
    "Script Error: BADVARIABLENAME", "Script Error: UNKNOWN_VARIABLE",    "Script Error: NOCOMMANDS",
    "Script Error: EXPRESSION",      "Script Error: MISSING_SETVARALIAS", "Script Error: WSTACKOVERFLOW",
    "Script Error: WSTACKUNDERFLOW", "Script Error: IFSTACKOVERFLOW",     "Script Error: IFSTACKUNDERFLOW",
    "Script Error: OUTOFMEMORY",     "Script Error: UNHANDLEDCOMMAND",    "Script Error: FILE_TYPE",
    "Script Error: FILE_CORRUPT",    "Script Error: LINE_TOO_LONG",       "Script Error: UNKNOWN_OBJECT"
};

// Indexed by EXPRESSION_OPERATORS.
using scriptOperators_t = SCRIPT_OPERATOR[EXP_COUNT];
inline scriptOperators_t scriptOperators{{EXP_LEFTPAREN, 0, "("},     {EXP_RIGHTPAREN, 0, ")"}, {EXP_AND, 1, "&&"},
                                         {EXP_OR, 2, "||"},           {EXP_LESSEQUAL, 3, "<="}, {EXP_LESS, 3, "<"},
                                         {EXP_GREATEREQUAL, 3, ">="}, {EXP_GREATER, 3, ">"},    {EXP_EQUAL, 3, "=="},
                                         {EXP_NOTEQUAL, 3, "!="},     {EXP_SUBTRACT, 4, "-"},   {EXP_ADD, 4, "+"},
                                         {EXP_MULTIPLY, 5, "*"},      {EXP_DIVIDE, 5, "/"},     {EXP_MODULUS, 5, "%"},
                                         {EXP_UNARYMINUS, 6, "~"}};
