/**
Integer (SSA value)
*/
typedef struct
{
    header_t header;

    int64_t val;

} ir_int_t;

/**
IR instruction (SSA value)
*/
typedef struct
{
    header_t header;

    // opcode
    // use word-sized struct, flags? later
    uint16_t opcode;

    /// Next instruction pointer
    struct ir_instr_t* next;

    // Keep track of uses? Needed for peephole/replacement

    // List of instruction arguments
    array_t* args;

} ir_instr_t;

/**
IR block, contains a sequence of instructions
*/
typedef struct
{
    header_t header;

    // List of instructions
    array_t* instrs;

    // TODO: branch targets
    // should belong to block rather than instrs?
    // unless they are just instr args

} ir_block_t;

/**
Function definition, contains multiple IR blocks
*/
typedef struct
{
    header_t header;

    // TODO:
    // List of parameter names (strings)


    // TODO:
    // List of parameter SSA values


    // Entry block
    ir_block_t* entry;

    // List of blocks
    array_t* blocks;

} ir_fun_t;

