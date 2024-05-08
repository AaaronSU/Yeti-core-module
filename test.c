#include <stdint.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>

#include <stdlib.h>

#include "vm.c"

#define MAX_INTERATION 1000
#define SIZE_FILE (SIZE_INSTRUCTION * MAX_INTERATION)


// Test sur instruction
/*
instruction_set[13] = movu;
    instruction_set[19] = movui;
    instruction_set[31] = addu;
    instruction_set[39] = incu;

    instruction_set[74] = cmpu;
    instruction_set[80] = jl;
    instruction_set[84] = outu;

    instruction_set[88] = outb;
    instruction_set[90] = hlt;
*/

static core_t *init_core()
{
    core_t *core = (core_t *)malloc(sizeof(core_t));
    if (core == NULL)
    {
        printf("failed to allocate core of size %zu bytes", sizeof(core_t));
        exit(1);
    }
    u8 *memory = (u8 *)malloc(MAX_MEMORY_SIZE);
    if (memory == NULL)
    {
        printf("failed to allocate memory of size %u bytes", MAX_MEMORY_SIZE);
        free(core);
        exit(1);
    }

    u8 *file_buffer = (u8 *)malloc(SIZE_FILE);

    if (file_buffer == NULL)
    {
        printf("failed to allocate file buffer of size %u bytes", SIZE_FILE);
        free(memory);
        free(core);
        exit(1);
    }
    core->memory = memory;
    core->file_buffer = file_buffer;

    for (int i = 0; i < NUMBER_SCALAR_REGISTER; ++i)
    {
        core->U[i] = 0;
        core->S[i] = 0;
        core->F[i] = 0.0;
    }

    for (int i = 0; i < 8; i++)
    {
        core->CF[i] = false;
    }

    core->IP = 0;

    return core;
}

static u32 create_instruction(u8 opcode, u16 offset, u8 register_1, u8 register_2, u8 register_3)
{
    return htobe32(opcode << (SIZE_INSTRUCTION - 8) | offset << (SIZE_INSTRUCTION - 17) | register_1 << (SIZE_INSTRUCTION - 22) | register_2 << (SIZE_INSTRUCTION - 27) | register_3);
}

static void test_movu(void **state)
{
    core_t *core = init_core();
    
    for (int i = 0; i < MAX_INTERATION; ++i)
    {
        int register_1 = random() % 32;
        int register_2 = random() % 32;

        u32 instruction = create_instruction(0, 0, register_1, register_2, 0);
        u32 *ptr = (u32 *)(core->file_buffer + (i * sizeof(u32)));

        *ptr = instruction;
        core->U[register_1] = random();
        int val_r2 = (core->U[register_1] == 0) ? 1 : core->U[register_1] / 2;
        core->U[register_2] = val_r2;

        movu(core);

        assert_int_equal(core->U[register_1], core->U[register_2]);
        assert_int_equal(val_r2, core->U[register_2]);
    }

}

int main(void)
{
    int result = 0;
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_movu),
    };
    result |= cmocka_run_group_tests_name("vm", tests, NULL, NULL);

    return result;
}