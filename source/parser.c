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

/// Shape indices for AST nodes
/// These are initialized in init_parser(), see parser.c
shapeidx_t SHAPE_AST_CONST;
shapeidx_t SHAPE_AST_REF;
shapeidx_t SHAPE_AST_DECL;
shapeidx_t SHAPE_AST_BINOP;
shapeidx_t SHAPE_AST_UNOP;
shapeidx_t SHAPE_AST_SEQ;
shapeidx_t SHAPE_AST_IF;
shapeidx_t SHAPE_AST_CALL;
shapeidx_t SHAPE_AST_FUN;

/**
Initialize data needed by the Zeta core parser
*/
void parser_init()
{
    // TODO: use shapes to describe AST node struct layouts
    // Just dummy shapes for now
    SHAPE_AST_CONST = shape_alloc_empty()->idx;
    SHAPE_AST_REF = shape_alloc_empty()->idx;
    SHAPE_AST_DECL = shape_alloc_empty()->idx;
    SHAPE_AST_BINOP = shape_alloc_empty()->idx;
    SHAPE_AST_UNOP = shape_alloc_empty()->idx;
    SHAPE_AST_SEQ = shape_alloc_empty()->idx;
    SHAPE_AST_IF = shape_alloc_empty()->idx;
    SHAPE_AST_CALL = shape_alloc_empty()->idx;
    SHAPE_AST_FUN = shape_alloc_empty()->idx;






}

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
        if (input_match_str(input, "/*"))
        {
            // Read until the end of the comment
            for (;;)
            {
                char ch = input_read_ch(input);
                if (ch == '*' && input_match_ch(input, '/'))
                    break;
            }

            continue;
        }

        // This isn't whitespace, stop
        break;
    }
}

/// Allocate an integer node
heapptr_t ast_const_alloc(value_t val)
{
    ast_const_t* node = (ast_const_t*)vm_alloc(
        sizeof(ast_const_t),
        SHAPE_AST_CONST
    );
    node->val = val;
    return (heapptr_t)node;
}

/// Allocate a reference node
heapptr_t ast_ref_alloc(heapptr_t name_str)
{
    ast_ref_t* node = (ast_ref_t*)vm_alloc(
        sizeof(ast_ref_t),
        SHAPE_AST_REF
    );
    assert (get_shape(name_str) == SHAPE_STRING);
    node->name = (string_t*)name_str;
    node->idx = 0xFFFF;
    return (heapptr_t)node;
}

/// Allocate a declaration node
heapptr_t ast_decl_alloc(heapptr_t name_str, bool cst)
{
    ast_decl_t* node = (ast_decl_t*)vm_alloc(
        sizeof(ast_decl_t),
        SHAPE_AST_DECL
    );
    assert (get_shape(name_str) == SHAPE_STRING);
    node->name = (string_t*)name_str;
    node->idx = 0xFFFF;
    node->cst = cst;
    node->capt = false;
    return (heapptr_t)node;
}

/// Allocate a binary operator node
heapptr_t ast_binop_alloc(
    const opinfo_t* op,
    heapptr_t left_expr,
    heapptr_t right_expr
)
{
    ast_binop_t* node = (ast_binop_t*)vm_alloc(
        sizeof(ast_binop_t),
        SHAPE_AST_BINOP
    );
    node->op = op;
    node->left_expr = left_expr;
    node->right_expr = right_expr;
    return (heapptr_t)node;
}

/// Allocate a unary operator node
heapptr_t ast_unop_alloc(
    const opinfo_t* op,
    heapptr_t expr
)
{
    ast_unop_t* node = (ast_unop_t*)vm_alloc(
        sizeof(ast_unop_t),
        SHAPE_AST_UNOP
    );
    node->op = op;
    node->expr = expr;
    return (heapptr_t)node;
}

/// Allocate an sequence expression node
heapptr_t ast_seq_alloc(
    array_t* expr_list
)
{
    ast_seq_t* node = (ast_seq_t*)vm_alloc(
        sizeof(ast_seq_t),
        SHAPE_AST_SEQ
    );
    node->expr_list = expr_list;
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
        SHAPE_AST_IF
    );
    node->test_expr = test_expr;
    node->then_expr = then_expr;
    node->else_expr = else_expr;
    return (heapptr_t)node;
}

/// Allocate a function call node
heapptr_t ast_call_alloc(
    heapptr_t fun_expr,
    array_t* arg_exprs
)
{
    ast_call_t* node = (ast_call_t*)vm_alloc(
        sizeof(ast_call_t),
        SHAPE_AST_CALL
    );
    node->fun_expr = fun_expr;
    node->arg_exprs = arg_exprs;
    return (heapptr_t)node;
}

/// Allocate a function expression node
heapptr_t ast_fun_alloc(
    array_t* param_decls,
    heapptr_t body_expr
)
{
    ast_fun_t* node = (ast_fun_t*)vm_alloc(
        sizeof(ast_fun_t),
        SHAPE_AST_FUN
    );
    node->param_decls = param_decls;
    node->body_expr = body_expr;
    return (heapptr_t)node;
}

/**
Parse an identifier
*/
heapptr_t parse_ident(input_t* input)
{
    size_t startIdx = input->idx;
    size_t len = 0;

    char firstCh = input_peek_ch(input);

    if (firstCh != '_' &&
        firstCh != '$' &&
        !isalpha(firstCh))
        return NULL;

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
Parse a number (integer or floating-point)
Note: floating-point numbers are not supported by the core parser
*/
heapptr_t parse_number(input_t* input)
{
    char* numStart;

    int64_t intVal;

    char* endInt = NULL;

    // Hexadecimal literals
    if (input_match_str(input, "0x"))
    {
        numStart = input->str->data + input->idx;
        intVal = strtol(numStart, &endInt, 16);
    }

    // Binary literals
    else if (input_match_str(input, "0b"))
    {
        numStart = input->str->data + input->idx;
        intVal = strtol(numStart, &endInt, 2);
    }

    // Decimal literals
    else
    {
        numStart = input->str->data + input->idx;
        intVal = strtol(numStart, &endInt, 10);
    }

    input->idx += endInt - numStart;
    return (heapptr_t)ast_const_alloc(value_from_int64(intVal));
}

/**
Parse a string value
*/
heapptr_t parse_string(input_t* input, char endCh)
{
    size_t len = 0;
    size_t cap = 64;

    char* buf = malloc(cap);

    for (;;)
    {
        // Consume this character
        char ch = input_read_ch(input);

        // If this is the end of the string
        if (ch == endCh)
        {
            break;
        }

        // If this is an escape sequence
        if (ch == '\\')
        {
            char esc = input_read_ch(input);

            switch (esc)
            {
                case 'n': ch = '\n'; break;
                case 'r': ch = '\r'; break;
                case 't': ch = '\t'; break;
                case '0': ch = '\0'; break;

                default:
                return NULL;
            }
        }

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

    buf[len] = '\0';

    // Get the interned version of this string
    return (heapptr_t)vm_get_cstr(buf);
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
    {
        input->error_str = "expected 'then' keyword";
        return NULL;
    }

    heapptr_t then_expr = parse_expr(input);

    // There must be a then clause
    if (then_expr == NULL)
    {
        return NULL;
    }

    heapptr_t else_expr;

    // If these is an else clause
    input_eat_ws(input);
    if (input_match_str(input, "else"))
    {
       else_expr = parse_expr(input);
    }
    else
    {
        else_expr = (heapptr_t)ast_const_alloc(VAL_FALSE);
    }

    return ast_if_alloc(test_expr, then_expr, else_expr);
}

/**
Parse a list of expressions
*/
array_t* parse_expr_list(input_t* input, char endCh, bool needSep)
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
        array_set_obj(arr, arr->len, expr);

        // Read whitespace
        input_eat_ws(input);

        // If this is the end of the list
        if (input_match_ch(input, endCh))
        {
            break;
        }

        // If this is not the first element, there must be a separator
        if (needSep && !input_match_ch(input, ','))
        {
            input->error_str = "expected comma separator in list";
            return NULL;
        }
    }

    return arr;
}

/**
Parse a function (closure) expression
fun (x,y,z) <body_expr>
*/
heapptr_t parse_fun_expr(input_t* input)
{
    input_eat_ws(input);
    if (!input_match_ch(input, '('))
    {
        input->error_str = "expected parameter list";
        return NULL;
    }

    // Allocate an array for the parameter declarations
    array_t* param_decls = array_alloc(4);

    // Until the end of the list
    for (;;)
    {
        // Read whitespace
        input_eat_ws(input);

        // If this is the end of the list
        if (input_match_ch(input, ')'))
            break;

        // Parse an identifier
        heapptr_t ident = parse_ident(input);

        if (ident == NULL)
            return NULL;

        heapptr_t decl = ast_decl_alloc(ident, false);

        // Write the expression to the array
        array_set_obj(param_decls, param_decls->len, decl);

        // Read whitespace
        input_eat_ws(input);

        // If this is the end of the list
        if (input_match_ch(input, ')'))
            break;

        // If this is not the first element, there must be a separator
        if (!input_match_ch(input, ','))
        {
            input->error_str = "expected comma separator in parameter list";
            return NULL;
        }
    }

    // Parse the function body
    heapptr_t body_expr = parse_expr(input);

    if (body_expr == NULL)
    {
        return NULL;
    }

    return (heapptr_t)ast_fun_alloc(param_decls, body_expr);
}

/// Member operator
const opinfo_t OP_MEMBER = { ".", NULL, 2, 16, 'l', false };

/// Array indexing
const opinfo_t OP_INDEX = { "[", "]", 2, 16, 'l', false };

/// Function call, variable arity
const opinfo_t OP_CALL = { "(", ")", -1, 15, 'l', false };

/// Prefix unary operators
const opinfo_t OP_NEG = { "-", NULL, 1, 13, 'r', false };
const opinfo_t OP_NOT = { "not", NULL, 1, 13, 'r', false };

/// Binary arithmetic operators
const opinfo_t OP_MUL = { "*", NULL, 2, 12, 'l', false };
const opinfo_t OP_DIV = { "/", NULL, 2, 12, 'l', true };
const opinfo_t OP_MOD = { "mod", NULL, 2, 12, 'l', true };
const opinfo_t OP_ADD = { "+", NULL, 2, 11, 'l', false };
const opinfo_t OP_SUB = { "-", NULL, 2, 11, 'l', true };

/// Relational operators
const opinfo_t OP_LT = { "<", NULL, 2, 9, 'l', false };
const opinfo_t OP_LE = { "<=", NULL, 2, 9, 'l', false };
const opinfo_t OP_GT = { ">", NULL, 2, 9, 'l', false };
const opinfo_t OP_GE = { ">=", NULL, 2, 9, 'l', false };
const opinfo_t OP_IN = { "in", NULL, 2, 9, 'l', false };
const opinfo_t OP_INST_OF = { "instanceof", NULL, 2, 9, 'l', false };

/// Equality comparison
const opinfo_t OP_EQ = { "==", NULL, 2, 8, 'l', false };
const opinfo_t OP_NE = { "!=", NULL, 2, 8, 'l', false };

/// Bitwise operators
const opinfo_t OP_BIT_AND = { "&", NULL, 2, 7, 'l', false };
const opinfo_t OP_BIT_XOR = { "^", NULL, 2, 6, 'l', false };
const opinfo_t OP_BIT_OR = { "|", NULL, 2, 5, 'l', false };

/// Logical operators
const opinfo_t OP_AND = { "and", NULL, 2, 4, 'l', false };
const opinfo_t OP_OR = { "or", NULL, 2, 3, 'l', false };

// Assignment
const opinfo_t OP_ASSG = { "=", NULL, 2, 1, 'r', false };

/**
Try to match an operator in the input
*/
const opinfo_t* input_match_op(input_t* input, int minPrec, bool preUnary)
{
    input_t beforeOp = *input;

    char ch = input_peek_ch(input);

    const opinfo_t* op = NULL;

    // Switch on the first character of the operator
    // We do this to avoid a long cascade of match tests
    switch (ch)
    {
        case '.':
        if (input_match_ch(input, '.'))     op = &OP_MEMBER;
        break;

        case '[':
        if (input_match_ch(input, '['))     op = &OP_INDEX;
        break;

        case '(':
        if (input_match_ch(input, '('))     op = &OP_CALL;
        break;

        case 'n':
        if (input_match_str(input, "not"))  op = &OP_NOT;
        break;

        case '*':
        if (input_match_ch(input, '*'))     op = &OP_MUL;
        break;

        case '/':
        if (input_match_ch(input, '/'))     op = &OP_DIV;
        break;

        case 'm':
        if (input_match_str(input, "mod"))  op = &OP_MOD;
        break;

        case '+':
        if (input_match_ch(input, '+'))     op = &OP_ADD;
        break;

        case '-':
        if (input_match_ch(input, '-'))
            op = preUnary? &OP_NEG:&OP_SUB;
        break;

        case '<':
        if (input_match_str(input, "<="))   op = &OP_LE;
        if (input_match_ch(input, '<'))     op = &OP_LT;
        break;

        case '>':
        if (input_match_str(input, ">="))   op = &OP_GE;
        if (input_match_ch(input, '>'))     op = &OP_GT;
        break;

        case 'i':
        if (input_match_str(input, "instanceof")) op = &OP_INST_OF;
        if (input_match_str(input, "in")) op = &OP_IN;
        break;

        case '=':
        if (input_match_str(input, "=="))   op = &OP_EQ;
        if (input_match_ch(input, '='))     op = &OP_ASSG;
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

    // If any operator was found
    if (op)
    {
        // If its precedence isn't high enough or it doesn't meet
        // the arity and associativity requirements
        if ((op->prec < minPrec) ||
            (preUnary && op->arity != 1) || 
            (preUnary && op->assoc != 'r'))
        {
            // Backtrack to avoid consuming the operator
            *input = beforeOp;
            op = NULL;
        }
    }

    // Return the matched operator, if any
    return op;
}

/**
Parse a variable declaration
Note: assumes that the "var" keyword has already been matched
*/
heapptr_t parse_var_decl(input_t* input)
{
    input_eat_ws(input);

    heapptr_t ident = parse_ident(input);

    if (ident == NULL)
    {
        input->error_str = "expected identifier in variable expression";
        return NULL;
    }

    return ast_decl_alloc(ident, false);
}

/**
Parse a constant declaration
Note: assumes that the "let" keyword has already been matched
*/
heapptr_t parse_cst_decl(input_t* input)
{
    input_eat_ws(input);

    heapptr_t ident = parse_ident(input);

    if (ident == NULL)
    {
        input->error_str = "expected identifier in variable expression";
        return NULL;
    }

    input_eat_ws(input);

    // A value must be assigned to the constant declared
    if (!input_match_str(input, "="))
    {
        input->error_str = "expected value assignment in let declaration";
        return NULL;
    }

    heapptr_t val = parse_expr(input);

    if (!val)
    {
        return NULL;
    }

    // Create and return an assignment expression
    return ast_binop_alloc(
        &OP_ASSG,
        ast_decl_alloc(ident, true),
        val
    );
}

/**
Parse an atomic expression
*/
heapptr_t parse_atom(input_t* input)
{
    //printf("parse_atom\n");

    // Consume whitespace
    input_eat_ws(input);

    // Numerical constant
    if (isdigit(input_peek_ch(input)))
    {
        return parse_number(input);
    }

    // String literal
    if (input_match_ch(input, '\''))
    {
        return parse_string(input, '\'');
    }
    if (input_match_ch(input, '"'))
    {
        return parse_string(input, '"');
    }

    // Array literal
    if (input_match_ch(input, '['))
    {
        return (heapptr_t)parse_expr_list(input, ']', true);
    }

    // Parenthesized expression
    if (input_match_ch(input, '('))
    {
        heapptr_t expr = parse_expr(input);
        if (expr == NULL)
        {
            input->error_str = "expected expression after '('\n";
            return NULL;
        }

        if (!input_match_ch(input, ')'))
        {
            return NULL;
        }

        return expr;
    }

    // Sequence/block expression (i.e { a; b; c }
    if (input_match_ch(input, '{'))
    {
        return ast_seq_alloc(parse_expr_list(input, '}', false));
    }

    // Try matching a right-associative (prefix) unary operators
    const opinfo_t* op = input_match_op(input, 0, true);

    // If a matching operator was found
    if (op)
    {
        heapptr_t expr = parse_atom(input);
        if (expr == NULL)
        {
            printf("expected atomic expression after prefix unary op\n");
            return NULL;
        }

        return (heapptr_t)ast_unop_alloc(op, expr);
    }

    // Identifier
    if (isalnum(input_peek_ch(input)))
    {
        // Variable declaration
        if (input_match_str(input, "var"))
            return parse_var_decl(input);

        // Constant declaration
        if (input_match_str(input, "let"))
            return parse_cst_decl(input);

        // If expression
        if (input_match_str(input, "if"))
            return parse_if_expr(input);

        // Function expression
        if (input_match_str(input, "fun"))
            return parse_fun_expr(input);

        // true and false boolean constants
        if (input_match_str(input, "true"))
            return (heapptr_t)ast_const_alloc(VAL_TRUE);
        if (input_match_str(input, "false"))
            return (heapptr_t)ast_const_alloc(VAL_FALSE);
    }

    // Identifiers beginning with non-alphanumeric characters
    if (input_peek_ch(input) == '_' ||
        input_peek_ch(input) == '$' ||
        isalpha(input_peek_ch(input)))
    {
        return ast_ref_alloc(parse_ident(input));
    }

    // Parsing failed
    return NULL;
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
    heapptr_t lhs_expr = parse_atom(input);

    if (lhs_expr == NULL)
    {
        return NULL;
    }

    for (;;)
    {
        // Consume whitespace
        input_eat_ws(input);

        //printf("looking for op, minPrec=%d\n", minPrec);

        // Attempt to match an operator in the input
        // with sufficient precedence
        const opinfo_t* op = input_match_op(input, minPrec, false);

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
            array_t* arg_exprs = parse_expr_list(input, ')', true);

            lhs_expr = ast_call_alloc(lhs_expr, arg_exprs);
        }

        // If this is a member expression
        else if (op == &OP_MEMBER)
        {
            // Parse the identifier string
            heapptr_t ident = parse_ident(input);

            if (ident == NULL)
            {
                input->error_str = "expected identifier in member expression";
                return NULL;
            }

            // Produce an indexing expression
            lhs_expr = ast_binop_alloc(
                op,
                lhs_expr,
                ident/*, lhs_expr.pos*/
            );
        }

        // If this is a binary operator
        else if (op->arity == 2)
        {
            // Recursively parse the rhs
            heapptr_t rhs_expr = parse_expr_prec(input, nextMinPrec);

            // The rhs expression must parse correctly
            if (rhs_expr == NULL)
                return NULL;

            // Create a new parent node for the expressions
            lhs_expr = ast_binop_alloc(
                op,
                lhs_expr,
                rhs_expr/*, lhs_expr.pos*/
            );

            // If specified, match the operator closing string
            if (op->close_str && !input_match_str(input, op->close_str))
                return NULL;
        }

        // If this is a unary operator
        else if (op->arity == 1)
        {
            if (op->assoc != 'l')
            {
                return NULL;
            }

            printf("postfix unary operator\n");
            assert (false);

            // Update lhs with the new value
            //lhs_expr = new UnOpExpr(op, lhs_expr, lhs_expr.pos);
        }

        else
        {
            // Unhandled operator
            printf("operator not handled correctly: %s\n", op->str);
            assert (false);
        }
    }

    // Return the parsed expression
    return lhs_expr;
}

/// Parse an expression
heapptr_t parse_expr(input_t* input)
{
    return parse_expr_prec(input, 0);
}

/// Parse a source unit
ast_fun_t* parse_unit(input_t* input)
{
    // Allocate an array with an initial capacity
    array_t* arr = array_alloc(32);

    // Until the end of the input is reached
    for (;;)
    {
        // Parse one expression
        heapptr_t expr = parse_expr(input);

        if (expr == NULL)
        {
            return NULL;
        }

        // Write the expression to the array
        array_set_obj(arr, arr->len, expr);

        // If this is the end of the input, stop
        input_eat_ws(input);
        if (input_eof(input))
            break;
    }

    // Create a sequence expression from the expression list
    heapptr_t seq_expr = ast_seq_alloc(arr);

    // Create an empty array for the parameter list
    array_t* param_list = array_alloc(0);

    return (ast_fun_t*)ast_fun_alloc(param_list, seq_expr);
}

/// Test that the parsing of a source unit succeeds
void test_parse(char* cstr)
{
    //printf("%s\n", cstr);

    string_t* str = string_alloc(strlen(cstr));
    strcpy(str->data, cstr);
    input_t input = input_from_string(str);

    ast_fun_t* unit = parse_unit(&input);

    // Consume any remaining whitespace
    input_eat_ws(&input);

    if (!unit)
    {
        printf("failed to parse:\n\"%s\"\n", cstr);
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

/// Test that the parsing of a source unit fails
void test_parse_fail(char* cstr)
{
    //printf("%s\n", cstr);

    string_t* str = string_alloc(strlen(cstr));
    strcpy(str->data, cstr);
    input_t input = input_from_string(str);

    ast_fun_t* unit = parse_unit(&input);

    // Consume any remaining whitespace
    input_eat_ws(&input);

    if (unit && input_eof(&input))
    {
        printf("parsing did not fail for:\n\"%s\"\n", cstr);
        exit(-1);
    }
}

/// Test the functionality of the parser
void test_parser()
{
    // Identifiers
    test_parse("foobar");
    test_parse("  foo_bar  ");
    test_parse("  foo_bar  ");
    test_parse("_foo");
    test_parse("$foo");
    test_parse("$foo52");

    // Literals
    test_parse("123");
    test_parse("0xFF");
    test_parse("0b101");
    test_parse("'abc'");
    test_parse("\"double-quoted string!\"");
    test_parse("\"double-quoted string, 'hi'!\"");
    test_parse("'hi' // comment");
    test_parse("'hi'");
    test_parse("'new\\nline'");
    test_parse("true");
    test_parse("false");
    test_parse_fail("'invalid\\iesc'");
    test_parse_fail("'str' []");

    // Array literals
    test_parse("[]");
    test_parse("[1]");
    test_parse("[1,a]");
    test_parse("[1 , a]");
    test_parse("[1,a, ]");
    test_parse("[ 1,\na ]");
    test_parse_fail("[,]");

    // Comments
    test_parse("1 // comment");
    test_parse("[ 1//comment\n,a ]");
    test_parse("1 /* comment */ + x");
    test_parse("1 /* // comment */ + x");
    test_parse_fail("1 // comment\n#1");
    test_parse_fail("1 /* */ */");

    // Arithmetic expressions
    test_parse("a + b");
    test_parse("a + b + c");
    test_parse("a + b - c");
    test_parse("a + b * c + d");
    test_parse("a or b or c");
    test_parse("(a)");
    test_parse("(a + b)");
    test_parse("(a + (b + c))");
    test_parse("((a + b) + c)");
    test_parse("(a + b) * (c + d)");
    test_parse_fail("*a");
    test_parse_fail("a*");
    test_parse_fail("a # b");
    test_parse_fail("a +");
    test_parse_fail("a + b # c");
    test_parse_fail("(a");
    test_parse_fail("(a + b))");
    test_parse_fail("((a + b)");

    // Member expression
    test_parse("a.b");
    test_parse("a.b + c");
    test_parse("$runtime.v0.add");
    test_parse("$api.file.v2.fopen");
    test_parse_fail("a.'b'");

    // Array indexing
    test_parse("a[0]");
    test_parse("a[b]");
    test_parse("a[b+2]");
    test_parse("a[2*b+1]");
    test_parse_fail("a[]");
    test_parse_fail("a[0 1]");

    // If expression
    test_parse("if x then y");
    test_parse("if x then y + 1");
    test_parse("if x then y else z");
    test_parse("if x then a+c else d");
    test_parse("if x then a else b");
    test_parse("if a instanceof b then true");
    test_parse("if 'a' in b or 'c' in b then y");
    test_parse("if not x then y else z");
    test_parse("if x and not x then true else false");
    test_parse("if x <= 2 then y else z");
    test_parse("if x == 1 then y+z else z+d");
    test_parse("if true then y else z");
    test_parse("if true or false then y else z");
    test_parse_fail("if x");
    test_parse_fail("if x then");
    test_parse_fail("if x then a if");

    // Assignment
    test_parse("x = 1");
    test_parse("x = -1");
    test_parse("a.b = x + y");
    test_parse("x = y = 1");
    test_parse("var x");
    test_parse("var x = 3");
    test_parse("let x=3");
    test_parse("let x= 3+y");
    test_parse_fail("var");
    test_parse_fail("let");
    test_parse_fail("let x");
    test_parse_fail("let x=");
    test_parse_fail("var +");
    test_parse_fail("var 3");

    // Call expressions
    test_parse("a()");
    test_parse("a(b)");
    test_parse("a(b,c)");
    test_parse("a(b,c+1)");
    test_parse("a(b,c+1,)");
    test_parse("x + a(b,c+1)");
    test_parse("x + a(b,c+1) + y");
    test_parse("a() b()");
    test_parse_fail("a(b c+1)");

    // Function expression
    test_parse("fun () 0");
    test_parse("fun (x) x");
    test_parse("fun (x,y) x");
    test_parse("fun (x,y,) x");
    test_parse("fun (x,y) x+y");
    test_parse("fun (x,y) if x then y else 0");
    test_parse("obj.method = fun (this, x) this.x = x");
    test_parse("let f = fun () 0\nf()");
    test_parse_fail("fun (x,y)");
    test_parse_fail("fun ('x') x");
    test_parse_fail("fun (x+y) y");

    // Fibonacci
    test_parse("let fib = fun (n) if n < 2 then n else fib(n-1) + fib(n-2)");

    // Sequence/block expression
    test_parse("{ a b }");
    test_parse("fun (x) { println(x) println(y) }");
    test_parse("fun (x) { var y = x + 1 print(y) }");
    test_parse("if (x) then { println(x) } else { println(y) z }");
    test_parse_fail("{ a, b }");

    // TODO: try parsing parser.zeta




}

