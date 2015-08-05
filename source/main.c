#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "vm.h"
#include "parser.h"
#include "interp.h"

int main(int argc, char** argv)
{
    vm_init();

    if (argc == 2)
    {
        printf("reading file \"%s\"\n", argv[1]);

        FILE* file = fopen(argv[1], "r");

        if (!file)
        {
            printf("failed to open file\n");
            return -1;
        }

        // Get the file size in bytes
        fseek(file, 0, SEEK_END);
        size_t len = ftell(file);
        fseek(file, 0, SEEK_SET);

        printf("%ld bytes\n", len);

        // Allocate the string object
        string_t* str = string_alloc(len);

        // Read into the allocated buffer
        fread(str->data, 1, len, file);

        // Close the input file
        fclose(file);

        // Create an input stream object
        input_t input = input_from_string(str);

        // Until the end of the input is reached
        while (!input_eof(&input))
        {
            // Parse one expression
            heapptr_t expr = parseExpr(&input);

            // Evaluate the expression
            evalExpr(expr);
        }
    }

    return 0;
}

