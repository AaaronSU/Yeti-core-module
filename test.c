#include <stdint.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "vm.c"

#define MAX_INTERATION 1000
#define SIZE_FILE ((sizeof(u32) + sizeof(u64)) * MAX_INTERATION)

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

        u32 *ptr_inst = (u32 *)(core->file_buffer + core->IP);
        u64 *ptr_imm = (u64 *)(core->file_buffer + core->IP + sizeof(u32));

        *ptr_inst = create_instruction(0, 0, r1, 0, 0);
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

        u32 *ptr = (u32 *)(core->file_buffer + core->IP);
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

        // CF[0] = 1 si opération = 0
        if (core->U[r1] == 0)
        {
            assert_true(core->CF[0]);
        }
    }
    core_drop(core);
}

static void test_incu(void **state)
{
    core_t *core = core_init();
    for (int i = 0; i < MAX_INTERATION; ++i)
    {
        u8 r1 = random() % 32;
        u64 v1 = random();
        core->U[r1] = v1;

        u32 *ptr = (u32 *)(core->file_buffer + core->IP);
        *ptr = create_instruction(0, 0, r1, 0, 0);

        incu(core);

        assert_int_equal(core->U[r1], v1 + 1);
    }
    core_drop(core);
}

static void test_cmpu(void **state)
{
    core_t *core = core_init();

    // U[r1] == U[r2]
    for (int i = 0; i < MAX_INTERATION; ++i)
    {
        
        u8 r1 = rand() % 32;
        u8 r2 = rand() % 32;

        u64 v1 = rand();

        core->U[r1] = v1;
        core->U[r2] = v1;

        u32 *ptr = (u32 *)(core->file_buffer + core->IP);
        *ptr = create_instruction(0, 0, r1, r2, 0);

        cmpu(core);

        assert_true(core->CF[7]);
        assert_false(core->CF[6]);
        assert_false(core->CF[5]);
        assert_true(core->CF[4]);
        assert_true(core->CF[2]);
        assert_false(core->CF[1]);
        assert_false(core->CF[0]);
    }

    // U[r1] > U[r2]
    for (int i = 0; i < MAX_INTERATION; ++i)
    { 
        u8 r1 = rand() % 32;
        u8 temp = rand() % 32;
        u8 r2 = (temp == r1) ? (r1 + 1) % 32 : temp;

        u64 v1 = (rand() % (RAND_MAX / 2)) + (RAND_MAX / 2);
        u64 v2 = (rand() % (RAND_MAX / 2));

        core->U[r1] = v1;
        core->U[r2] = v2;

        core->IP = 0;

        u32 *ptr = (u32 *)(core->file_buffer + core->IP);
        *ptr = create_instruction(0, 0, r1, r2, 0);

        cmpu(core);

        assert_false(core->CF[7]);
        assert_true(core->CF[6]);
        assert_true(core->CF[5]);
        assert_true(core->CF[4]);
        assert_false(core->CF[2]);
        assert_false(core->CF[1]);
        assert_false(core->CF[0]);
    }
    
    core_drop(core);
}

static void test_jl(void **state)
{
    core_t *core = core_init();
    // dépassement ? 
    // A GERER ==> DONNE PROBLEME MALLOC DEVRAIT GERER LE FAIT DE DEPASSER, MESSAGE DERREUR OU modulo?? 

    // Test sans dépassement
    for (int i = 0; i < MAX_INTERATION; ++i)
    {
        core->IP = 0;
        core->CF[3] = true;

        // TODO : vérifier que ça dépasse vraiment pas la taille du buffer
        u8 size = ((rand() * SIZE_INSTRUCTION_IN_BYTE) % SIZE_FILE);

        u64 *ptr_addr = (u64 *)(core->file_buffer + core->IP + sizeof(u32));

        *ptr_addr = htobe64(size);
        jl(core);

        assert_int_equal(core->IP, size);
    }

    core_drop(core);
}

static void test_outu(void** state)
{
    /* core_t* core = core_init();
    u8 * chaine = "hello";
    core->U[0] = (u64)(*chaine);

    u64 *ptr = (u64 *)(core->file_buffer + core->IP);
    *ptr = 0;

    int out = open("./out.dat", O_CREAT | O_WRONLY | O_TRUNC, 0600); // peut creer des problemes
    dup2(out, STDOUT_FILENO); // aussi
    outu(core);
    dup2(STDOUT_FILENO, out);

    close(out);

    core_drop(core); */
}

int main(void)
{
    int result = 0;
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_movu),
        cmocka_unit_test(test_movui),
        cmocka_unit_test(test_addu),
        cmocka_unit_test(test_incu),
        cmocka_unit_test(test_cmpu),
        cmocka_unit_test(test_jl),
        //cmocka_unit_test(test_outu),
    };
    result |= cmocka_run_group_tests_name("vm", tests, NULL, NULL);

    return result;
}