#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "vm.h"
#include "parser.h"
#include "interp.h"

// TODO: move to interp.c
/// Evaluate the code in a given string
void eval_string(char* cstr)
{
    size_t len = strlen(cstr);

    // Allocate a hosted string object
    string_t* str = string_alloc(len);

    strncpy(str->data, cstr, len);

    // Create a parser input stream object
    input_t input = input_from_string(str);

    // Until the end of the input is reached
    for (;;)
    {
        // If this is the end of the input, stop
        input_eat_ws(&input);
        if (input_eof(&input))
            break;

        // Parse one expression
        heapptr_t expr = parse_expr(&input);

        if (expr == NULL)
        {
            printf("parse failed at idx %d\n", input.idx);
            printf("char: %c\n", input_peek_ch(&input));

            return;
        }

        // Evaluate the expression
        eval_expr(expr);
    }
}

/// Read a text file
char* read_file(char* file_name)
{
    printf("reading file \"%s\"\n", file_name);

    FILE* file = fopen(file_name, "r");

    if (!file)
    {
        printf("failed to open file\n");
        return NULL;
    }

    // Get the file size in bytes
    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    fseek(file, 0, SEEK_SET);

    printf("%ld bytes\n", len);

    char* buf = malloc(len);

    // Read into the allocated buffer
    int read = fread(buf, 1, len, file);

    if (read != len)
    {
        printf("failed to read file");
        return NULL;
    }

    // Close the input file
    fclose(file);

    return buf;
}

/// Read a line from standard input
char* read_line()
{
    size_t cap = 256;
    size_t len = 0;
    char* buf = malloc(cap+1);

    for (;;)
    {
        char ch = getchar();

        if (ch == '\0')
            return 0;

        if (ch == '\n')
            break;

        buf[len] = ch;
        len++;

        if (len == cap)
        {
            cap *= 2;
            char* new_buf = malloc(cap+1);
            strncpy(new_buf, buf, len);
            free(buf);
            buf = new_buf;
        }
    }

    // Add a null terminator to the string
    buf[len] = '\0';

    return buf;
}

int main(int argc, char** argv)
{
    vm_init();

    // Test mode
    if (argc == 2 && strcmp(argv[1], "--test") == 0)
    {
        test_parser();
        test_interp();
        return 0;
    }

    // File name passed
    if (argc == 2)
    {
        char* cstr = read_file(argv[1]);

        if (cstr == NULL)
            return -1;

        // Evaluate the code string
        eval_string(cstr);

        free(cstr);
    }

    // Read-eval-print loop
    if (argc == 1 /*|| "--e"*/)
    {
        for (;;)
        {
            char* cstr = read_line();

            // Evaluate the code string
            eval_string(cstr);

            free(cstr);
        }
    }

    return 0;
}

