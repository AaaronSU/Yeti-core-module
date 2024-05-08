#include <stdint.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>

#include <stdlib.h>

#include "vm.c"

#define MAX_INTERATION 1000
#define SIZE_FILE ((sizeof(u32) + sizeof(u64)) * MAX_INTERATION)

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

static core_t *core_init()
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
        printf("failed to allocate file buffer of size %lu bytes", SIZE_FILE);
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
    core_t *core = core_init();

    // U[register_1] = U[register_2]
    for (int i = 0; i < MAX_INTERATION; ++i)
    {
        // Peut-être le meme registre, pas de soucis avec ça.
        u8 register_1 = random() % 32;
        u8 register_2 = random() % 32;

        u32 instruction = create_instruction(0, 0, register_1, register_2, 0);
        u32 *ptr = (u32 *)(core->file_buffer + core->IP);

        *ptr = instruction;

        u64 val_r2 = random();
        core->U[register_2] = val_r2;

        if (register_1 != register_2)
        {
            assert_int_not_equal(core->U[register_1], core->U[register_2]);
        }

        movu(core);
        assert_int_equal(core->U[register_1], core->U[register_2]);
        assert_int_equal(val_r2, core->U[register_2]);
    }

    // cas extreme des u64 ?

    // cas ou c'est pas des u64 ?
    core_drop(core);
}

static void test_movui(void **state)
{
    core_t *core = core_init();

    // U[r1] = imm (test quand U[r1] = 0 et U[r1] =! 0 si MAX_ITERATION grand)
    for (int i = 0; i < MAX_INTERATION; ++i)
    {
        u8 r1 = random() % 32;
        u64 imm = random();

        u32 instruction = create_instruction(0, 0, r1, 0, 0);

        u32 *ptr_inst = (u32 *)(core->file_buffer + core->IP);
        u64 *ptr_imm = (u64 *)(core->file_buffer + core->IP + sizeof(u32));

        *ptr_inst = instruction;
        *ptr_imm = htobe64(imm);

        movui(core);
        assert_int_equal(core->U[r1], imm);
    }

    core_drop(core);
}

static void test_addu(void **state)
{
    core_t *core = core_init();
    for (int i = 0; i < MAX_INTERATION; ++i)
    {
        u8 r1 = random() % 32;
        u8 r2 = random() % 32;
        u8 r3 = random() % 32;

        u64 v2 = random();
        u64 v3 = random();

        core->U[r2] = v2;
        core->U[r3] = v3;

        u32* ptr = (u32 *)(core->file_buffer + core->IP);
        *ptr = create_instruction(0, 0, r1, r2, r3);

        addu(core);
        
        if (r2 == r3)
        {
            assert_int_equal(core->U[r1], v3 + v3);
        }
        else
        {
            assert_int_equal(core->U[r1], (v2 + v3));
        }
        
    }
    core_drop(core);
}

int main(void)
{
    int result = 0;
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_movu),
        cmocka_unit_test(test_movui),
        cmocka_unit_test(test_addu),
    };
    result |= cmocka_run_group_tests_name("vm", tests, NULL, NULL);

    return result;
}