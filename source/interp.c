#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <alloca.h>
#include "interp.h"
#include "parser.h"
#include "vm.h"

/// Shape indices for mutable cells and closures
/// These are initialized in init_interp(), see interp.c
shapeidx_t SHAPE_CELL;
shapeidx_t SHAPE_CLOS;

/**
Initialize the interpreter
*/
void interp_init()
{
    SHAPE_CELL = shape_alloc_empty()->idx;
    SHAPE_CLOS = shape_alloc_empty()->idx;
}

cell_t* cell_alloc()
{
    cell_t* cell = (cell_t*)vm_alloc(sizeof(cell_t), SHAPE_CELL);

    return cell;
}

clos_t* clos_alloc(ast_fun_t* fun)
{
    clos_t* clos = (clos_t*)vm_alloc(
        sizeof(clos_t) + sizeof(cell_t*) * fun->capt_vars->len,
        SHAPE_STRING
    );

    return clos;
}

void find_decls(heapptr_t expr, ast_fun_t* fun)
{
    // Get the shape of the AST node
    shapeidx_t shape = get_shape(expr);

    if (shape == SHAPE_AST_DECL)
    {
        ast_decl_t* decl = (ast_decl_t*)expr;

        // If this variable is already declared, do nothing
        for (size_t i = 0; i < fun->local_decls->len; ++i)
        {
            ast_decl_t* local = (ast_decl_t*)array_get_ptr(fun->local_decls, i);
            if (local->name == decl->name)
                return;
        }

        decl->idx = fun->local_decls->len;
        array_set_obj(fun->local_decls, decl->idx, (heapptr_t)decl);

        /*
        printf("found decl\n");
        string_print(decl->name);
        printf("\n");
        */

        return;
    }

    // Sequence/block expression
    if (shape == SHAPE_AST_SEQ)
    {
        ast_seq_t* seqexpr = (ast_seq_t*)expr;
        array_t* expr_list = seqexpr->expr_list;

        for (size_t i = 0; i < expr_list->len; ++i)
            find_decls(array_get(expr_list, i).word.heapptr, fun);

        return;
    }

    // Binary operator (e.g. a + b)
    if (shape == SHAPE_AST_BINOP)
    {
        ast_binop_t* binop = (ast_binop_t*)expr;

        find_decls(binop->left_expr, fun);
        find_decls(binop->right_expr, fun);
        return;
    }

    // TODO: complete, add assertion
}

void var_res(heapptr_t expr, ast_fun_t* fun)
{
    // Get the shape of the AST node
    shapeidx_t shape = get_shape(expr);

    if (shape == SHAPE_AST_REF)
    {
        ast_ref_t* ref = (ast_ref_t*)expr;

        // For each scope
        ast_fun_t* cur;
        for (cur = fun; cur != NULL; cur = cur->parent)
        {
            // For each local
            for (size_t i = 0; i < cur->local_decls->len; ++i)
            {
                ast_decl_t* local = (ast_decl_t*)array_get_ptr(cur->local_decls, i);

                if (local->name == ref->name)
                {
                    // If the variable is from this scope
                    if (cur == fun)
                    {
                        ref->idx = local->idx;
                    }

                    // The variable is from an outer scope
                    else
                    {
                        // Mark the declaration as captured
                        local->capt = true;

                        // Thread the local as a closure variable
                        ast_fun_t* clos;
                        for (clos = fun; clos != cur; clos = clos->parent)
                        {
                            array_set_obj(
                                clos->capt_vars, 
                                clos->capt_vars->len,
                                (heapptr_t)local
                            );
                        }
                    }

                    return;
                }
            }
        }

        /*
        printf("found global\n");
        string_print(ref->name);
        printf("\n");
        */

        // If unresolved, mark as global
        ref->global = true;

        return;
    }

    // Sequence/block expression
    if (shape == SHAPE_AST_SEQ)
    {
        ast_seq_t* seqexpr = (ast_seq_t*)expr;
        array_t* expr_list = seqexpr->expr_list;

        for (size_t i = 0; i < expr_list->len; ++i)
            var_res(array_get_ptr(expr_list, i), fun);

        return;
    }

    // Binary operator (e.g. a + b)
    if (shape == SHAPE_AST_BINOP)
    {
        ast_binop_t* binop = (ast_binop_t*)expr;

        var_res(binop->left_expr, fun);
        var_res(binop->right_expr, fun);
        return;
    }

    // TODO: complete, add assertion
}

/**
Resolve variables in a given function
*/
void var_res_pass(ast_fun_t* fun, ast_fun_t* parent)
{
    fun->parent = parent;

    // Add the function parameters to the local scope
    for (size_t i = 0; i < fun->param_decls->len; ++i)
    {
        array_set(fun->local_decls, i, array_get(fun->param_decls, i));
    }

    // Find declarations in the function body
    find_decls(fun->body_expr, fun);

    // Resolve variable references
    var_res(fun->body_expr, fun);
}

/**
Evaluate the boolean value of a value
Note: the semantics of boolean evaluation are intentionally
kept strict in the core language.
*/
bool eval_truth(value_t value)
{
    switch (value.tag)
    {
        case TAG_BOOL:
        return value.word.int8 != 0;

        default:
        printf("cannot use value as boolean\n");
        exit(-1);
    }
}

/**
Evaluate an assignment expression
*/
value_t eval_assign(
    heapptr_t lhs_expr,
    heapptr_t rhs_expr,
    ast_fun_t* fun,
    value_t* locals
)
{
    value_t val = eval_expr(rhs_expr, fun, locals);

    shapeidx_t shape = get_shape(lhs_expr);

    // Assignment to variable declaration
    if (shape == SHAPE_AST_DECL)
    {
        ast_decl_t* decl = (ast_decl_t*)lhs_expr;

        // If this variable is captured by a closure
        if (decl->capt)
        {
            // Closure variables are stored in mutable cells
            // Pointers to the cells are stored on the stack



            // TODO
            assert (false);

            return val;
        }

        locals[decl->idx] = val;

        return val;
    }

    // Assignment to a variable
    if (shape == SHAPE_AST_REF)
    {
        ast_ref_t* ref = (ast_ref_t*)lhs_expr;

        // If this a closure variable
        if (ref->capt)
        {
            // Closure variables are stored in mutable cells
            // Pointers to the cells are found on the closure object




            // TODO
            assert (false);

            return val;
        }

        // If this is a global variable
        if (ref->global)
        {
            // TODO
            assert (false);

            return val;
        }

        return val;
    }

    printf("\n");
    exit(-1);
}

/**
Evaluate an expression in a given frame
*/
value_t eval_expr(
    heapptr_t expr, 
    ast_fun_t* fun,
    value_t* locals
)
{
    // Get the shape of the AST node
    // Note: AST nodes must match the shapes defined in init_parser,
    // otherwise this interpreter can't handle it
    shapeidx_t shape = get_shape(expr);

    // Variable reference
    if (shape == SHAPE_AST_REF)
    {
        ast_ref_t* ref = (ast_ref_t*)expr;

        // If this is a captured (closure) variable
        if (ref->capt)
        {
            // TODO
            assert (false);








        }

        // TODO: handle globals
        assert (!ref->global);

        return locals[ref->idx];
    }

    if (shape == SHAPE_AST_CONST)
    {
        ast_const_t* cst = (ast_const_t*)expr;
        return cst->val;
    }

    if (shape == SHAPE_STRING)
    {
        return value_from_heapptr(expr, TAG_STRING);
    }

    // Array literal expression
    if (shape == SHAPE_ARRAY)
    {
        array_t* array_expr = (array_t*)expr;

        array_t* val_array = array_alloc(array_expr->len);

        for (size_t i = 0; i < array_expr->len; ++i)
        {
            heapptr_t expr = array_get(array_expr, i).word.heapptr;
            value_t value = eval_expr(expr, fun, locals);
            array_set(val_array, i, value);
        }

        return value_from_heapptr((heapptr_t)val_array, TAG_ARRAY);
    }

    // Binary operator (e.g. a + b)
    if (shape == SHAPE_AST_BINOP)
    {
        ast_binop_t* binop = (ast_binop_t*)expr;

        // Assignment
        if (binop->op == &OP_ASSIGN)
        {
            return eval_assign(
                binop->left_expr, 
                binop->right_expr, 
                fun,
                locals
            );
        }

        value_t v0 = eval_expr(binop->left_expr, fun, locals);
        value_t v1 = eval_expr(binop->right_expr, fun, locals);
        int64_t i0 = v0.word.int64;
        int64_t i1 = v1.word.int64;

        if (binop->op == &OP_INDEX)
            return array_get((array_t*)v0.word.heapptr, i1);

        if (binop->op == &OP_ADD)
            return value_from_int64(i0 + i1);
        if (binop->op == &OP_SUB)
            return value_from_int64(i0 - i1);
        if (binop->op == &OP_MUL)
            return value_from_int64(i0 * i1);
        if (binop->op == &OP_DIV)
            return value_from_int64(i0 / i1);
        if (binop->op == &OP_MOD)
            return value_from_int64(i0 % i1);

        if (binop->op == &OP_LT)
            return (i0 < i1)? VAL_TRUE:VAL_FALSE;
        if (binop->op == &OP_LE)
            return (i0 <= i1)? VAL_TRUE:VAL_FALSE;
        if (binop->op == &OP_GT)
            return (i0 > i1)? VAL_TRUE:VAL_FALSE;
        if (binop->op == &OP_GE)
            return (i0 >= i1)? VAL_TRUE:VAL_FALSE;

        if (binop->op == &OP_EQ)
            return (memcmp(&v0, &v1, sizeof(v0)) == 0)? VAL_TRUE:VAL_FALSE;
        if (binop->op == &OP_NE)
            return (memcmp(&v0, &v1, sizeof(v0)) != 0)? VAL_TRUE:VAL_FALSE;

        printf("unimplemented binary operator: %s\n", binop->op->str);
        return VAL_FALSE;
    }

    // Unary operator (e.g.: -x, not a)
    if (shape == SHAPE_AST_UNOP)
    {
        ast_unop_t* unop = (ast_unop_t*)expr;

        value_t v0 = eval_expr(unop->expr, fun, locals);

        if (unop->op == &OP_NEG)
            return value_from_int64(-v0.word.int64);

        if (unop->op == &OP_NOT)
            return eval_truth(v0)? VAL_FALSE:VAL_TRUE;

        printf("unimplemented unary operator: %s\n", unop->op->str);
        return VAL_FALSE;
    }

    // Sequence/block expression
    if (shape == SHAPE_AST_SEQ)
    {
        ast_seq_t* seqexpr = (ast_seq_t*)expr;
        array_t* expr_list = seqexpr->expr_list;

        value_t value;

        for (size_t i = 0; i < expr_list->len; ++i)
        {
            heapptr_t expr = array_get(expr_list, i).word.heapptr;
            value = eval_expr(expr, fun, locals);
        }

        // Return the value of the last expression
        return value;
    }

    // If expression
    if (shape == SHAPE_AST_IF)
    {
        ast_if_t* ifexpr = (ast_if_t*)expr;

        value_t t = eval_expr(ifexpr->test_expr, fun, locals);

        if (eval_truth(t))
            return eval_expr(ifexpr->then_expr, fun, locals);
        else
            return eval_expr(ifexpr->else_expr, fun, locals);
    }

    // Call expression
    if (shape == SHAPE_AST_CALL)
    {
        ast_call_t* callexpr = (ast_call_t*)expr;
        heapptr_t clos_expr = callexpr->fun_expr;
        array_t* arg_exprs = callexpr->arg_exprs;

        // Evaluate the closure expression
        value_t clos_val = eval_expr(clos_expr, fun, locals);

        if (clos_val.tag != TAG_CLOS)
        {
            printf("expected closure in function call\n");
            exit(-1);
        }

        clos_t* clos = clos_val.word.clos;
        ast_fun_t* fun = clos->fun;

        if (arg_exprs->len != fun->param_decls->len)
        {
            printf("argument count mismatch\n");
            exit(-1);
        }

        // Allocate space for the local variables
        value_t* locals = alloca(sizeof(value_t) * fun->local_decls->len);

        // TODO: evaluate the arguments (later)

        // TODO: allocate closure cells for the captured variables (later)

        // Evaluate the unit function body in the local frame
        return eval_expr(fun->body_expr, fun, locals);
    }

    // Function/closure expression
    if (shape == SHAPE_AST_FUN)
    {
        ast_fun_t* fun = (ast_fun_t*)expr;

        // Allocate a closure of the function
        clos_t* clos = clos_alloc(fun);

        return value_from_heapptr((heapptr_t)clos, TAG_CLOS);
    }

    printf("eval error, unknown expression type, shapeidx=%d\n", get_shape(expr));
    exit(-1);
}

/**
Evaluate the source code in a given string
This can also be used to evaluate files
*/
value_t eval_str(const char* cstr, const char* src_name)
{
    // Create a parser input stream object
    input_t input = input_from_string(
        vm_get_cstr(cstr),
        vm_get_cstr(src_name)
    );

    // Parse the input as a source code unit
    ast_fun_t* unit_fun = parse_unit(&input);

    // Resolve all variables in the unit
    var_res_pass(unit_fun, NULL);

    // Allocate space for the local variables
    value_t* locals = alloca(sizeof(value_t) * unit_fun->local_decls->len);

    // TODO: allocate the closure cells



    // Evaluate the unit function body in the local frame
    return eval_expr(unit_fun->body_expr, unit_fun, locals);
}

void test_eval(char* cstr, value_t expected)
{
    value_t value = eval_str(cstr, "test");

    if (!value_equals(value, expected))
    {
        printf(
            "value doesn't match expected for input:\n%s\n",
            cstr
        );

        exit(-1);
    }
}

void test_eval_int(char* cstr, int64_t expected)
{
    test_eval(cstr, value_from_int64(expected));
}

void test_eval_true(char* cstr)
{
    test_eval(cstr, VAL_TRUE);
}

void test_eval_false(char* cstr)
{
    test_eval(cstr, VAL_FALSE);
}

void test_interp()
{
    test_eval_int("0", 0);
    test_eval_int("1", 1);
    test_eval_int("7", 7);
    test_eval_int("0xFF", 255);
    test_eval_int("0b101", 5);
    test_eval_true("true");
    test_eval_false("false");

    // Arithmetic
    test_eval_int("3 + 2 * 5", 13);
    test_eval_int("-7", -7);
    test_eval_int("-(7 + 3)", -10);
    test_eval_int("3 + -2 * 5", -7);

    // Comparisons
    test_eval_true("0 < 5");
    test_eval_true("0 <= 5");
    test_eval_true("0 <= 0");
    test_eval_true("0 == 0");
    test_eval_true("0 != 1");
    test_eval_true("not false");
    test_eval_true("not not true");
    test_eval_true("true == true");
    test_eval_false("true == false");
    test_eval_true("'foo' == 'foo'");
    test_eval_false("'foo' == 'bar'");
    test_eval_true("'f' != 'b'");
    test_eval_false("'f' != 'f'");

    // Arrays
    test_eval_int("[7][0]", 7);
    test_eval_int("[0,1,2][0]", 0);
    test_eval_int("[7+3][0]", 10);

    // Sequence expression
    test_eval_int("{ 2 3 }", 3);
    test_eval_int("{ 2 3+7 }", 10);
    test_eval_int("3 7", 7);

    // If expression
    test_eval_int("if true then 1 else 0", 1);
    test_eval_int("if false then 1 else 0", 0);
    test_eval_int("if 0 < 10 then 7 else 3", 7);
    test_eval_int("if not true then 1 else 0", 0);

    // Variable declarations
    test_eval_int("var x = 3\nx", 3);
    test_eval_int("let x = 7\nx+1", 8);

    // Closures
    test_eval_true("fun () 1\ntrue");
    test_eval_true("let f = fun () 1\ntrue");

    // FIXME: segfaults
    //test_eval_int("let f = fun () 1\nf()", 1);





}

