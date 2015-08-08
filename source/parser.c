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

char* srcpos_to_str(srcpos_t pos, char* buf)
{
    sprintf(buf, "@%d:%d", pos.lineNo, pos.colNo);
    return buf;
}

input_t input_from_string(string_t* str)
{
    input_t input;
    input.str = str;
    input.idx = 0;
    input.pos.lineNo = 0;
    input.pos.colNo = 0;
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

    if (ch == '\n')
    {
        input->pos.lineNo++;
        input->pos.colNo = 0;
    }
    else
    {
        input->pos.colNo++;
    }

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
heapptr_t ast_int_alloc(int64_t val)
{
    ast_int_t* node = (ast_int_t*)vm_alloc(sizeof(ast_int_t), TAG_AST_INT);
    node->val = val;
    return (heapptr_t)node;
}

/// Allocate a binary operator node
heapptr_t ast_binop_alloc(
    const opinfo_t* op,
    heapptr_t left,
    heapptr_t right
)
{
    ast_binop_t* node = (ast_binop_t*)vm_alloc(
        sizeof(ast_binop_t),
        TAG_AST_BINOP
    );
    node->op = op;
    node->left = left;
    node->right = right;
    return (heapptr_t)node;
}

/// Allocate an if expression node
heapptr_t ast_if_alloc(
    heapptr_t test_expr,
    heapptr_t then_expr,
    heapptr_t else_expr
)
{
    ast_if_t* node = (ast_if_t*)vm_alloc(
        sizeof(ast_if_t),
        TAG_AST_IF
    );
    node->test_expr = test_expr;
    node->then_expr = then_expr;
    node->else_expr = else_expr;
    return (heapptr_t)node;
}

/// Allocate a function call node
heapptr_t ast_call_alloc(
    heapptr_t fun_expr,
    heapptr_t arg_exprs
)
{
    ast_call_t* node = (ast_call_t*)vm_alloc(
        sizeof(ast_call_t),
        TAG_AST_CALL
    );
    node->fun_expr = fun_expr;
    node->arg_exprs = arg_exprs;
    return (heapptr_t)node;
}

/**
Parse an identifier
*/
heapptr_t parse_ident(input_t* input)
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
heapptr_t parse_int(input_t* input)
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
heapptr_t parse_str(input_t* input)
{
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
Parse an if expression
if <test_expr> then <then_expr> else <else_expr>
*/
heapptr_t parse_if_expr(input_t* input)
{
    heapptr_t test_expr = parse_expr(input);

    input_eat_ws(input);
    if (!input_match_str(input, "then"))
        return NULL;

    heapptr_t then_expr = parse_expr(input);

    input_eat_ws(input);
    if (!input_match_str(input, "else"))
        return NULL;

    heapptr_t else_expr = parse_expr(input);

    return ast_if_alloc(test_expr, then_expr, else_expr);
}

/**
Parse a list of expressions
*/
heapptr_t parse_expr_list(input_t* input, char endCh)
{
    // Allocate an array with an initial capacity
    array_t* arr = array_alloc(4);

    // Until the end of the list
    for (;;)
    {
        // Read whitespace
        input_eat_ws(input);

        // If this is the end of the list
        if (input_match_ch(input, endCh))
        {
            break;
        }

        // Parse an expression
        heapptr_t expr = parse_expr(input);

        // The expression must not fail to parse
        if (expr == NULL)
        {
            return NULL;
        }

        // Write the expression to the array
        array_set_ptr(arr, arr->len, expr);

        // Read whitespace
        input_eat_ws(input);

        // If this is the end of the list
        if (input_match_ch(input, endCh))
        {
            break;
        }

        // If this is not the first element, there must be a comma
        if (!input_match_ch(input, ','))
        {
            // TODO: return a parse error node
            // all the way to the top?
            return NULL;
        }
    }

    return (heapptr_t)arr;
}

/**
Parse an atomic expression
*/
heapptr_t parseAtom(input_t* input)
{
    // Consume whitespace
    input_eat_ws(input);

    // Identifier
    if (isalnum(input_peek_ch(input)))
    {
        // if expression
        if (input_match_str(input, "if"))
            return parse_if_expr(input);

        return parse_ident(input);
    }

    // Integer constant
    if (isdigit(input_peek_ch(input)))
        return parse_int(input);

    // String literal
    if (input_match_ch(input, '\''))
        return parse_str(input);

    // Array literal
    if (input_match_ch(input, '['))
        return parse_expr_list(input, ']');

    // skip for now, no type tags, don't want true/false AST nodes
    // true and false boolean constants
    if (input_match_str(input, "true"))
        assert (false);
    if (input_match_str(input, "false"))
        assert (false);

    // TODO: parenthesized expression

    // Parsing failed
    return NULL;
}

// Member operator
//{ "."w, 2, 16, 'l' },

// Array indexing
const opinfo_t OP_IDX = { "[", "]", 2, 16, 'l', false };

// Function call, variable arity
const opinfo_t OP_CALL = { "(", ")", -1, 15, 'l', false };

// Prefix unary operators
const opinfo_t OP_NEG = { "-", NULL, 1, 13, 'r', false };
const opinfo_t OP_NOT = { "NOT", NULL, 1, 13, 'r', false };

// Binary arithmetic operators
const opinfo_t OP_MUL = { "*", NULL, 2, 12, 'l', false };
const opinfo_t OP_DIV = { "/", NULL, 2, 12, 'l', false };
const opinfo_t OP_MOD = { "%", NULL, 2, 12, 'l', false };
const opinfo_t OP_ADD = { "+", NULL, 2, 11, 'l', false };
const opinfo_t OP_SUB = { "-", NULL, 2, 11, 'l', false };

// Relational operators
/*
{ "<"w         , 2, IN_PREC, 'l' },
{ "<="w        , 2, IN_PREC, 'l' },
{ ">"w         , 2, IN_PREC, 'l' },
{ ">="w        , 2, IN_PREC, 'l' },
{ "in"w        , 2, IN_PREC, 'l' },
*/

// Equality comparison
const opinfo_t OP_EQ = { "==", NULL, 2, 8, 'l', false };
const opinfo_t OP_NE = { "!=", NULL, 2, 8, 'l', false };

/*
// Bitwise operators
{ "&"w, 2, 7, 'l' },
{ "^"w, 2, 6, 'l' },
{ "|"w, 2, 5, 'l' },
*/

// Logical operators
const opinfo_t OP_AND = { "and", NULL, 2, 4, 'l', false };
const opinfo_t OP_OR = { "or", NULL, 2, 3, 'l', false };

/**
Try to match an operator in the input
*/
const opinfo_t* input_match_op(input_t* input, int minPrec)
{
    input_t beforeOp = *input;

    char ch = input_peek_ch(input);

    const opinfo_t* op = NULL;

    // Switch on the first character of the operator
    // We do this to avoid a long cascade of match tests
    switch (ch)
    {
        case '[':
        if (input_match_ch(input, '['))     op = &OP_IDX;
        break;

        case '(':
        if (input_match_ch(input, '('))     op = &OP_CALL;
        break;

        case '*':
        if (input_match_ch(input, '*'))     op = &OP_MUL;
        break;

        case '/':
        if (input_match_ch(input, '/'))     op = &OP_DIV;
        break;

        case '%':
        if (input_match_ch(input, '%'))     op = &OP_MOD;
        break;

        case '+':
        if (input_match_ch(input, '+'))     op = &OP_ADD;
        break;

        case '-':
        if (input_match_ch(input, '-'))     op = &OP_SUB;
        break;

        case '=':
        if (input_match_str(input, "=="))   op = &OP_EQ;
        break;

        case '!':
        if (input_match_str(input, "!="))   op = &OP_NE;
        break;

        case 'a':
        if (input_match_str(input, "and"))  op = &OP_AND;
        break;

        case 'o':
        if (input_match_str(input, "or"))   op = &OP_OR;
        break;
    }

    // If any operator was found but its precedence isn't high enough
    if (op && op->prec < minPrec)
    {
        // Backtrack to avoid consuming the operator
        *input = beforeOp;
        op = NULL;
    }

    // Return the matched operator, if any
    return op;
}

/**
Parse an expression using the precedence climbing algorithm
*/
heapptr_t parse_expr_prec(input_t* input, int minPrec)
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

    // Parse the first atom
    heapptr_t lhsExpr = parseAtom(input);

    for (;;)
    {
        // Consume whitespace
        input_eat_ws(input);

        //printf("looking for op, minPrec=%d\n", minPrec);

        // Attempt to match an operator in the input
        // with sufficient precedence
        const opinfo_t* op = input_match_op(input, minPrec);

        // If no operator matches, break out
        if (op == NULL)
            break;

        //printf("found op: %s\n", op->str);
        //printf("op->prec=%d, minPrec=%d\n", op->prec, minPrec);

        // Compute the minimal precedence for the recursive call (if any)
        int nextMinPrec;
        if (op->assoc == 'l')
        {
            if (op->close_str)
                nextMinPrec = 0;
            else
                nextMinPrec = (op->prec + 1);
        }
        else
        {
            nextMinPrec = op->prec;
        }

        // If this is a function call expression
        if (op == &OP_CALL)
        {
            // Parse the argument list and create the call expression
            heapptr_t argExprs = parse_expr_list(input, ')');

            lhsExpr = ast_call_alloc(lhsExpr, argExprs);
        }

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
        else if (op->arity == 2)
        {
            // Recursively parse the rhs
            heapptr_t rhsExpr = parse_expr_prec(input, nextMinPrec);

            // The rhs expression must parse correctly
            if (rhsExpr == NULL)
                return NULL;

            // Create a new parent node for the expressions
            lhsExpr = ast_binop_alloc(
                op,
                lhsExpr,
                rhsExpr/*, lhsExpr.pos*/
            );

            // If specified, match the operator closing string
            if (op->close_str && !input_match_str(input, op->close_str))
                return NULL;
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

    // Return the parsed expression
    return lhsExpr;
}

/// Parse an expression
heapptr_t parse_expr(input_t* input)
{
    return parse_expr_prec(input, 0);
}

/// Test that the parsing of an expression succeeds or fails
void test_parse_expr(char* cstr, bool shouldParse)
{
    //printf("testing parse of %s\n", cstr);

    string_t* str = string_alloc(strlen(cstr));
    strcpy(str->data, cstr);
    input_t input = input_from_string(str);

    heapptr_t expr = parse_expr(&input);

    // Consume any remaining whitespace
    input_eat_ws(&input);

    if (shouldParse)
    {
        if (!expr)
        {
            printf("failed to produce an expression from:\n\"%s\"\n", cstr);
            exit(-1);
        }

        if (!input_eof(&input))
        {
            printf(
                "unconsumed input:\n\"%s\"\nremains for:\n\"%s\"\n",
                &cstr[input.idx],
                cstr
            );
            exit(-1);
        }
    }
    else 
    {
        if (!shouldParse && expr && input_eof(&input))
        {
            printf("parsing did not fail for:\n\"%s\"\n", cstr);
            exit(-1);
        }
    }
}

/// Test the functionality of the parser
void test_parser()
{
    test_parse_expr("123", true);
    test_parse_expr("foobar", true);
    test_parse_expr("  foo_bar  ", true);
    test_parse_expr("'abc'", true);
    test_parse_expr("'hi' // comment", true);
    test_parse_expr("'hi'", true);

    test_parse_expr("[]", true);
    test_parse_expr("[1]", true);
    test_parse_expr("[,]", false);
    test_parse_expr("[1,a]", true);
    test_parse_expr("[1 , a]", true);
    test_parse_expr("[1,a, ]", true);
    test_parse_expr("[ 1,\na ]", true);
    test_parse_expr("[ 1//comment\n,a ]", true);
    test_parse_expr("'str' []", false);

    // Arithmetic expressions
    test_parse_expr("a + b", true);
    test_parse_expr("a + b + c", true);
    test_parse_expr("a + b - c", true);
    test_parse_expr("a + b * c + d", true);
    test_parse_expr("a or b or c", true);

    // Malformed arithmetic expressions
    test_parse_expr("a # b", false);
    test_parse_expr("a +", false);
    test_parse_expr("a + b # c", false);

    // Array indexing
    test_parse_expr("a[0]", true);
    test_parse_expr("a[b]", true);
    test_parse_expr("a[b+2]", true);
    test_parse_expr("a[2*b+1]", true);
    test_parse_expr("a[]", false);
    test_parse_expr("a[0 1]", false);

    // If expression
    test_parse_expr("if x then y else z", true);
    test_parse_expr("if x then a+c else d", true);
    test_parse_expr("if x then a else b", true);
    test_parse_expr("if x == 1 then y+z else z+d", true);
    test_parse_expr("if x then a b", false);

    // Call expressions
    test_parse_expr("a()", true);
    test_parse_expr("a(b)", true);
    test_parse_expr("a(b,c)", true);
    test_parse_expr("a(b,c+1)", true);
    test_parse_expr("a(b,c+1,)", true);
    test_parse_expr("a(b c+1)", false);
    test_parse_expr("x + a(b,c+1)", true);
    test_parse_expr("x + a(b,c+1) + y", true);
}

