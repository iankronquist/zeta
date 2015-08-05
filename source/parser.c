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
void ast_binop_alloc()
{
    // TODO



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

/**
Parse an expression
*/
heapptr_t parseExpr(input_t* input)
{
    // TODO







    return parseAtom(input);



}

