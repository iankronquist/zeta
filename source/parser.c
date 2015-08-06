#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "parser.h"
#include "vm.h"

input_t input_from_string(string_t* str)
{
    input_t input;
    input.str = str;
    input.idx = 0;
    return input;
}

/// Test if the end of file has been reached
bool input_eof(input_t* input)
{
    assert (input->str != NULL);
    return (input->idx >= input->str->len);
}

/// Peek at a character from the input
char input_peek_ch(input_t* input)
{
    assert (input->str != NULL);

    if (input->idx >= input->str->len)
        return '\0';

    return input->str->data[input->idx];
}

/// Read a character from the input
char input_read_ch(input_t* input)
{
    char ch = input_peek_ch(input);
    input->idx++;
    return ch;
}

/// Try and match a given character in the input
/// The character is consumed if matched
bool input_match_ch(input_t* input, char ch)
{
    if (input_peek_ch(input) == ch)
    {
        input_read_ch(input);
        return true;
    }

    return false;
}

/// Try and match a given string in the input
/// The string is consumed if matched
bool input_match_str(input_t* input, char* str)
{
    input_t sub = *input;

    for (;;)
    {
        if (*str == '\0')
        {
            *input = sub;
            return true;
        }

        if (input_eof(&sub))
        {
            return false;
        }

        if (!input_match_ch(&sub, *str))
        {
            return false;
        }

        str++;
    }
}

/// Consume whitespace and comments
void input_eat_ws(input_t* input)
{
    // Until the end of the whitespace
    for (;;)
    {
        // Consume whitespace characters
        if (isspace(input_peek_ch(input)))
        {
            input_read_ch(input);
            continue;
        }

        // If this is a single-line comment
        if (input_match_str(input, "//"))
        {
            // Read until and end of line is reached
            for (;;)
            {
                char ch = input_read_ch(input);
                if (ch == '\n' || ch == '\0')
                    break;
            }

            continue;
        }

        // If this is a multi-line comment
        /*
        if (input_match_str(input, "/*"))
        {
            // TODO: handle nested comments

            // Read until and end of line is reached
            for (;;)
            {
                char ch = input_read_ch(input);
                if (ch == '\n' || ch == '\0')
                    break;
            }

            continue;
        }
        */

        // This isn't whitespace, stop
        break;
    }
}

/// Allocate an integer node
ast_int_t* ast_int_alloc(int64_t val)
{
    ast_int_t* node = (ast_int_t*)vm_alloc(sizeof(ast_int_t), DESC_AST_INT);
    node->val = val;
    return node;
}

/// Allocate a binary operator node
ast_binop_t* ast_binop_alloc(
    const opinfo_t* op,
    heapptr_t left,
    heapptr_t right
)
{
    ast_binop_t* node = (ast_binop_t*)vm_alloc(
        sizeof(ast_binop_t),
        DESC_AST_BINOP
    );
    node->op = op;
    node->left = left;
    node->right = right;
    return node;
}

/**
Parse an identifier
*/
heapptr_t parseIdent(input_t* input)
{
    size_t startIdx = input->idx;
    size_t len = 0;

    for (;;)
    {
        char ch = input_peek_ch(input);

        if (!isalnum(ch) && ch != '$' && ch != '_')
            break;

        // Consume this character
        input_read_ch(input);
        len++;
    }

    if (len == 0)
        return NULL;

    string_t* str = string_alloc(len);

    // Copy the characters
    strncpy(str->data, input->str->data + startIdx, len);

    return (heapptr_t)str;
}

/**
Parse an integer value
*/
heapptr_t parseInt(input_t* input)
{
    size_t numDigits = 0;
    int64_t intVal = 0;

    for (;;)
    {
        char ch = input_peek_ch(input);

        if (!isdigit(ch))
            break;

        intVal = 10 * intVal + (ch - '0');

        // Consume this digit
        input_read_ch(input);
        numDigits++;
    }

    if (numDigits == 0)
        return NULL;

    return (heapptr_t)ast_int_alloc(intVal);
}

/**
Parse a string value
*/
heapptr_t parseStr(input_t* input)
{
    // Strings should begin with a single quote
    char ch = input_read_ch(input);
    if (ch != '\'')
        return NULL;

    size_t len = 0;
    size_t cap = 64;

    char* buf = malloc(cap);

    for (;;)
    {
        // Consume this character
        char ch = input_read_ch(input);

        if (ch == '\'')
            break;

        buf[len] = ch;
        len++;

        if (len == cap)
        {
            cap *= 2;
            char* newBuf = malloc(cap);
            strncpy(newBuf, buf, len);
            free(buf);
        }
    }

    string_t* str = string_alloc(len);

    // Copy the characters
    strncpy(str->data, buf, len);

    return (heapptr_t)str;
}

/**
Parse a list of expressions
*/
heapptr_t parseExprList(input_t* input, char beginCh, char endCh)
{
    // Check for the list start character
    char ch = input_read_ch(input);
    if (ch != beginCh)
        return NULL;

    // Allocate an array with an initial capacity
    array_t* arr = array_alloc(4);

    // Until the end of the list
    for (;;)
    {
        // Read whitespace
        input_eat_ws(input);

        // Consume this character
        char ch = input_peek_ch(input);
        if (ch == endCh)
        {
            input_read_ch(input);
            break;
        }

        // Parse an expression
        heapptr_t expr = parseExpr(input);

        array_set_ptr(arr, arr->len, expr);
    }

    return (heapptr_t)arr;
}

/**
Parse an atomic expression
*/
heapptr_t parseAtom(input_t* input)
{
    printf("idx=%d\n", input->idx);

    // Consume whitespace
    input_eat_ws(input);

    input_t sub;
    heapptr_t expr;

    // Try parsing an identifier
    sub = *input;
    expr = parseIdent(&sub);
    if (expr != NULL)
    {
        *input = sub;
        return expr;
    }

    // Try parsing an integer
    sub = *input;
    expr = parseInt(&sub);
    if (expr != NULL)
    {
        *input = sub;
        return expr;
    }

    // Try parsing a string
    sub = *input;
    expr = parseStr(&sub);
    if (expr != NULL)
    {
        *input = sub;
        return expr;
    }

    // Try parsing an array
    sub = *input;
    expr = parseExprList(&sub, '[', ']');
    if (expr != NULL)
    {
        *input = sub;
        return expr;
    }

    // Parsing failed
    return NULL;
}

/// Operator definitions
const opinfo_t OP_ADD = { "+", NULL, 2, 11, 'l', false };
const opinfo_t OP_SUB = { "-", NULL, 2, 11, 'l', false };

// Maximum operator precedence
//const int MAX_PREC = 16;

/*
OpInfo[] operators = [

    // Member operator
    { "."w, 2, 16, 'l' },

    // TODO: end str
    // Array indexing
    { "["w, 1, 16, 'l' },

    // TODO: end str
    // Function call
    { "("w, 1, 15, 'l' },

    // Prefix unary operators
    { "+"w , 1, 13, 'r' },
    { "-"w , 1, 13, 'r' },
    { "not"w , 1, 13, 'r' },

    // Multiplication/division/modulus
    { "*"w, 2, 12, 'l' },
    { "/"w, 2, 12, 'l', true },
    { "%"w, 2, 12, 'l', true },

    // Addition/subtraction
    { "+"w, 2, 11, 'l' },
    { "-"w, 2, 11, 'l', true },

    // Relational operators
    { "<"w         , 2, IN_PREC, 'l' },
    { "<="w        , 2, IN_PREC, 'l' },
    { ">"w         , 2, IN_PREC, 'l' },
    { ">="w        , 2, IN_PREC, 'l' },
    { "in"w        , 2, IN_PREC, 'l' },
    { "instanceof"w, 2, IN_PREC, 'l' },

    // Equality comparison
    { "=="w , 2, 8, 'l' },
    { "!="w , 2, 8, 'l' },
    { "==="w, 2, 8, 'l' },
    { "!=="w, 2, 8, 'l' },

    // Bitwise operators
    { "&"w, 2, 7, 'l' },
    { "^"w, 2, 6, 'l' },
    { "|"w, 2, 5, 'l' },

    // Logical operators
    { "and"w, 2, 4, 'l' },
    { "or"w, 2, 3, 'l' },

    // Assignment
    { "="w   , 2, 1, 'r' },
];
*/

/**
Try to match an operator in the input
*/
const opinfo_t* input_match_op(input_t* input)
{
    char ch = input_peek_ch(input);

    // Switch on the character
    switch (ch)
    {
        case '+':
        if (input_match_str(input, "+"))
            return &OP_ADD;
        break;

        case '-':
        if (input_match_str(input, "-"))
            return &OP_SUB;
        break;
    }

    // No match found
    return NULL;
}

/**
Parse an expression using the precedence climbing algorithm
*/
heapptr_t parseExprPrec(input_t* input, int minPrec)
{
    // The first call has min precedence 0
    //
    // Each call loops to grab everything of the current precedence or
    // greater and builds a left-sided subtree out of it, associating
    // operators to their left operand
    //
    // If an operator has less than the current precedence, the loop
    // breaks, returning us to the previous loop level, this will attach
    // the atom to the previous operator (on the right)
    //
    // If an operator has the mininum precedence or greater, it will
    // associate the current atom to its left and then parse the rhs

    //writeln("parseExpr");

    // Parse the first atom
    heapptr_t lhsExpr = parseAtom(input);

    for (;;)
    {
        // Consume whitespace
        input_eat_ws(input);

        // Attempt to match an operator in the input
        const opinfo_t* op = input_match_op(input);

        // If no operator matches, break out
        if (op == NULL)
            break;

        // If the new operator has lower precedence, break out
        if (op->prec < minPrec)
            break;

        //writefln("binary op: %s", cur.stringVal);

        // Compute the minimal precedence for the recursive call (if any)
        int nextMinPrec = (op->assoc == 'l')? (op->prec + 1):op->prec;

        /*
        // If this is a function call expression
        if (cur.stringVal == "(")
        {
            // TODO: change parseExprList to not consume opening token,
            // since it's matched and consumed here

            // Parse the argument list and create the call expression
            auto argExprs = parseExprList(input, "(", ")");
            lhsExpr = new CallExpr(lhsExpr, argExprs, lhsExpr.pos);
        }
        */

        /*
        // If this is an array indexing expression
        else if (input.matchSep("["))
        {
            auto indexExpr = parseExpr(input);
            input.readSep("]");
            lhsExpr = new IndexExpr(lhsExpr, indexExpr, lhsExpr.pos);
        }
        */

        /*
        // If this is a member expression
        else if (op.str == ".")
        {
            input.read();

            // Parse the identifier string
            auto tok = input.read();
            if (!(tok.type is Token.IDENT) &&
                !(tok.type is Token.KEYWORD) &&
                !(tok.type is Token.OP && ident(tok.stringVal)))
            {
                throw new ParseError(
                    "invalid member identifier \"" ~ tok.toString() ~ "\"", 
                    tok.pos
                );
            }
            auto stringExpr = new StringExpr(tok.stringVal, tok.pos);

            // Produce an indexing expression
            lhsExpr = new IndexExpr(lhsExpr, stringExpr, lhsExpr.pos);
        }
        */

        // If this is a binary operator
        /*else*/ if (op->arity == 2)
        {
            // Recursively parse the rhs
            heapptr_t rhsExpr = parseExprPrec(input, nextMinPrec);

            // Update lhs with the new value
            lhsExpr = (heapptr_t)ast_binop_alloc(op, lhsExpr, rhsExpr/*, lhsExpr.pos*/);
        }

        /*
        // If this is a unary operator
        else if (op.arity == 1)
        {
            // Consume the current token
            input.read();

            // Update lhs with the new value
            lhsExpr = new UnOpExpr(op, lhsExpr, lhsExpr.pos);
        }
        */

        else
        {
            // Unhandled operator
            assert (false);
        }
    }

    //writeln("leaving parseExpr");

    // Return the parsed expression
    return lhsExpr;
}

/// Parse an expression
heapptr_t parseExpr(input_t* input)
{
    return parseExprPrec(input, 0);
}

