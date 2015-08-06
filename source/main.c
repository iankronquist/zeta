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

int main(int argc, char** argv)
{
    vm_init();

    // Test mode
    if (argc == 2 && strcmp(argv[1], "--test") == 0)
    {
        test_parser();
        return 0;
    }

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
        int read = fread(str->data, 1, len, file);

        if (read != len)
        {
            printf("failed to read file");
            return -1;
        }

        // Close the input file
        fclose(file);

        // Create an input stream object
        input_t input = input_from_string(str);

        // Until the end of the input is reached
        for (;;)
        {
            // Consume whitespace
            input_eat_ws(&input);

            // If this is the end of the input, stop
            if (input_eof(&input))
                break;

            // Parse one expression
            heapptr_t expr = parse_expr(&input);

            if (expr == NULL)
            {
                printf("parse failed at idx %d\n", input.idx);
                printf("char: %c\n", input_peek_ch(&input));

                return -1;
            }

            // Evaluate the expression
            eval_expr(expr);
        }
    }

    return 0;
}

