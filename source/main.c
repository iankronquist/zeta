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

void run_repl()
{
    printf("Zeta Read-Eval-Print Loop (REPL). Press Ctrl+C to exit.\n");
    printf("\n");
    printf("Please note that the Zeta VM is at the early prototype ");
    printf("stage, language semantics and implementation details will ");
    printf("change often.\n");
    printf("\n");
    printf("NOTE: the interpreter is currently *very much incomplete*. It will ");
    printf("likely crash on you or give cryptic error messages.\n");
    printf("\n");

    for (;;)
    {
        printf("z> ");

        char* cstr = read_line();

        // Evaluate the code string
        value_t value = eval_str(cstr, "shell");

        free(cstr);

        // Print the value
        value_print(value);
        putchar('\n');
    }
}

int main(int argc, char** argv)
{
    vm_init();
    parser_init();

    // Test mode
    if (argc == 2 && strcmp(argv[1], "--test") == 0)
    {
        test_vm();
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
        eval_str(cstr, argv[1]);

        free(cstr);
    }

    // No file names passed. Read-eval-print loop.
    if (argc == 1)
    {
        run_repl();
    }

    return 0;
}

