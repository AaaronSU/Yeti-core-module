#ifndef VM_H_
#define VM_H_

#include <stdbool.h>
#include <stdint.h>

#define PERF 0

#define NUMBER_SCALAR_REGISTER 32
#define NUMBER_VECTOR_REGISTER 32
#define SIZE_INSTRUCTION 32
// SIZE_INSTRUCTION_IN_BYTE = 4
#define SIZE_INSTRUCTION_IN_BYTE SIZE_INSTRUCTION / 8
#define SIZE_IMMEDIATE_IN_BYTE 8
#define MAX_INSTRUCTION_NUMBER 256
#define MAX_MEMORY_SIZE 1024 * 1024
#define MAX_FILE_BUFFER_SIZE 256
// IF YOU CHANGE MAX_FILE_NAME_SIZE, YOU NEED TO CHANGE ARGUMENTS OF FSCANF IN READ_CONFIG FUNCTION,
// OTHERWISE, THIS DOESN'T WORK PROPERLY
#define MAX_FILE_NAME_SIZE 256
#define info(fmt, ...)                                                      \
    do                                                                      \
    {                                                                       \
        fprintf(stderr, "\x1b[1;36m[INFO]:\x1b[0m " fmt "\n", __VA_ARGS__); \
    } while (false)

#define warn(fmt, ...)                                                         \
    do                                                                         \
    {                                                                          \
        fprintf(stderr, "\x1b[1;33m[WARNING]:\x1b[0m " fmt "\n", __VA_ARGS__); \
    } while (false)

#define error(fmt, ...)                                                      \
    do                                                                       \
    {                                                                        \
        fprintf(stderr, "\x1b[1;31m[ERROR]:\x1b[0m " fmt "\n", __VA_ARGS__); \
    } while (false)

#ifdef DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...) \
    do                   \
    {                    \
    } while (0)
#endif

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t i64;
typedef double f64;

typedef struct instruction_s
{
    u8 opcode;
    u16 offset;
    u8 register_1;
    u8 register_2;
    u8 register_3;

} instruction_t;

typedef struct header_s
{
    u64 magic_number;
    u64 header_size;
    u64 address_data;        // Data section address
    u64 size_data;           // Data section size
    u64 address_code;        // Code section address
    u64 size_code;           // Code section size
    u64 address_parallel_on; // Parallel on section address
    u64 size_parallel_on;
    u64 core_number;
    u64 size_total; // Size total
} header_t;

typedef struct core_s
{
    // scalar register
    u64 U[NUMBER_SCALAR_REGISTER];
    i64 S[NUMBER_SCALAR_REGISTER];
    f64 F[NUMBER_SCALAR_REGISTER];

    // vector register
    // u64 V[NUMBER_VECTOR_REGISTER][SIZE_VECTOR];
    // i64 T[NUMBER_VECTOR_REGISTER][SIZE_VECTOR];
    // f64 G[NUMBER_VECTOR_REGISTER][SIZE_VECTOR];

    // comparaison flag register
    bool CF[8];

    // Instruction pointer
    u64 IP;

    u8 *memory;
    char *file_buffer;
    header_t header;
    // FILE *fd;
    u16 id;
    u8 type; // coretype, 0 = compute, 1 = management
    // mutex
    // pthread_mutex_t mutex_tab[MAX_MUTEX_PER_CORE];

    // Pointer to the execution function
    // void (*insn_exec)(...);
} core_t;

instruction_t instruction_new(u32 instruction);

core_t *core_new(char *buffer, u16 id);

u64 get_immediate(core_t *core);

void loadu(core_t *core);

void storeu(core_t *core);

void movu(core_t *core);

void movui(core_t *core);

void addu(core_t *core);

void mulu(core_t *core);

void fmau(core_t *core);

void incu(core_t *core);

void cmpu(core_t *core);

void jl(core_t *core);

void outu(core_t *core);

void outb(core_t *core);

void hlt(core_t *core);

void undefined_instruction(core_t *core);

void core_execute(core_t *self);

void core_drop(core_t *self);

void set_up_instruction_set();

void read_config(char *config_file_name, char **file_buffer_list, u16 *number_of_file);

void excute_program();

#endif