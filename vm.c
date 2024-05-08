#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>

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

instruction_t instruction_new(u32 instruction)
{
    instruction = htobe32(instruction);
    return (instruction_t){
        .opcode = instruction >> (SIZE_INSTRUCTION - 8) & 0xFF,
        .offset = instruction >> (SIZE_INSTRUCTION - 17) & 0x1FF,
        .register_1 = instruction >> (SIZE_INSTRUCTION - 22) & 0x1F,
        .register_2 = instruction >> (SIZE_INSTRUCTION - 27) & 0x1F,
        .register_3 = instruction & 0x1F,
    };
}

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
    u8 *file_buffer;
    header_t header;
    // FILE *fd;
    u8 id;
    u8 type; // coretype, 0 = compute, 1 = management
    // mutex
    // pthread_mutex_t mutex_tab[MAX_MUTEX_PER_CORE];

    // Pointer to the execution function
    // void (*insn_exec)(...);
} core_t;

core_t *core_new(char *buffer, u8 id)
{
    core_t *core = (core_t *)malloc(sizeof(core_t)); // Allocate memory for the core
    if (core == NULL)
    {
        error("failed to allocate core of size %zu bytes", sizeof(core_t));
        exit(EXIT_FAILURE);
    }
    core->file_buffer = buffer;

    core->id = id;
    core->type = 0;
    for (int i = 0; i < NUMBER_SCALAR_REGISTER; i++)
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

    u8 *memory = (u8 *)malloc(MAX_MEMORY_SIZE);
    memset(memory, 0, MAX_MEMORY_SIZE);
    if (memory == NULL)
    {
        free(core);
        error("failed to allocate memory of size %zu bytes", sizeof(core_t));
        exit(EXIT_FAILURE);
    }
    core->memory = memory;

    return core;
}

u64 get_immediate(core_t *core)
{
    u64 immediate = htobe64(*(u64 *)&(core->file_buffer[core->IP]));
    core->IP += SIZE_IMMEDIATE_IN_BYTE;
    return immediate;
}

static void (*instruction_set[MAX_INSTRUCTION_NUMBER])(core_t *);

void loadu(core_t *core)
{
    instruction_t instruction = instruction_new(*(u32 *)&(core->file_buffer[core->IP]));
    DEBUG_PRINT("--------Avant LOADU--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "core->memory[**address**] a pour valeur %lx\n",
                instruction.register_1, core->U[instruction.register_1],
                instruction.register_2, core->U[instruction.register_2],
                instruction.register_3, core->U[instruction.register_3],
                *(u64 *)&(core->memory[core->U[instruction.register_2] + core->U[instruction.register_3] + instruction.offset]));
    core->U[instruction.register_1] = *(u64 *)&(core->memory[instruction.register_2 + instruction.register_3 + instruction.offset]);
    DEBUG_PRINT("--------Après LOADU--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "core->memory[**address**] a pour valeur %lx\n\n",
                instruction.register_1, core->U[instruction.register_1],
                instruction.register_2, core->U[instruction.register_2],
                instruction.register_3, core->U[instruction.register_3],
                *(u64 *)&(core->memory[core->U[instruction.register_2] + core->U[instruction.register_3] + instruction.offset]));
    core->IP += SIZE_INSTRUCTION_IN_BYTE;
}

void storeu(core_t *core)
{
    instruction_t instruction = instruction_new(*(u32 *)&(core->file_buffer[core->IP]));
    DEBUG_PRINT("--------Avant STOREU--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "core->memory[**address**] a pour valeur %ld\n",
                instruction.register_1, core->U[instruction.register_1],
                instruction.register_2, core->U[instruction.register_2],
                instruction.register_3, core->U[instruction.register_3],
                *(u64 *)&(core->memory[core->U[instruction.register_2] + core->U[instruction.register_3] + instruction.offset]));
    core->memory[core->U[instruction.register_2] + core->U[instruction.register_3] + instruction.offset] = core->U[instruction.register_1];
    DEBUG_PRINT("--------Après STOREU--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "core->memory[**address**] a pour valeur %ld\n\n",
                instruction.register_1, core->U[instruction.register_1],
                instruction.register_2, core->U[instruction.register_2],
                instruction.register_3, core->U[instruction.register_3],
                *(u64 *)&(core->memory[core->U[instruction.register_2] + core->U[instruction.register_3] + instruction.offset]));
    core->IP += SIZE_INSTRUCTION_IN_BYTE;
}

void movu(core_t *core)
{
    instruction_t instruction = instruction_new(*(u32 *)&(core->file_buffer[core->IP]));
    DEBUG_PRINT("--------Avant Mouvement--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n",
                instruction.register_1,
                core->U[instruction.register_1],
                instruction.register_2, core->U[instruction.register_2]);
    core->U[instruction.register_1] = core->U[instruction.register_2];
    DEBUG_PRINT("--------Après Mouvement--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n\n",
                instruction.register_1,
                core->U[instruction.register_1],
                instruction.register_2, core->U[instruction.register_2]);
    core->IP += SIZE_INSTRUCTION_IN_BYTE;
}

void movui(core_t *core)
{
    instruction_t instruction = instruction_new(*(u32 *)&(core->file_buffer[core->IP]));
    core->IP += SIZE_INSTRUCTION_IN_BYTE;
    u64 immediate = get_immediate(core);
    core->U[instruction.register_1] = immediate;
}

void addu(core_t *core)
{
    instruction_t instruction = instruction_new(*(u32 *)&(core->file_buffer[core->IP]));
    DEBUG_PRINT("--------Avant Addition--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n",
                instruction.register_1, core->U[instruction.register_1],
                instruction.register_2, core->U[instruction.register_2],
                instruction.register_3, core->U[instruction.register_3]);
    core->U[instruction.register_1] = core->U[instruction.register_2] + core->U[instruction.register_3];
    DEBUG_PRINT("--------Après Addition--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n\n",
                instruction.register_1, core->U[instruction.register_1],
                instruction.register_2, core->U[instruction.register_2],
                instruction.register_3, core->U[instruction.register_3]);
    core->IP += SIZE_INSTRUCTION_IN_BYTE;
}

void mulu(core_t *core)
{
    instruction_t instruction = instruction_new(*(u32 *)&(core->file_buffer[core->IP]));
    DEBUG_PRINT("--------Avant Multiplication--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n",
                instruction.register_1, core->U[instruction.register_1],
                instruction.register_2, core->U[instruction.register_2],
                instruction.register_3, core->U[instruction.register_3]);
    core->U[instruction.register_1] = core->U[instruction.register_2] * core->U[instruction.register_3];
    DEBUG_PRINT("--------Après Multiplication--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n\n",
                instruction.register_1, core->U[instruction.register_1],
                instruction.register_2, core->U[instruction.register_2],
                instruction.register_3, core->U[instruction.register_3]);
    core->IP += SIZE_INSTRUCTION_IN_BYTE;
}

void fmau(core_t *core)
{
    instruction_t instruction = instruction_new(*(u32 *)&(core->file_buffer[core->IP]));
    DEBUG_PRINT("--------Avant FMA--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n",
                instruction.register_1, core->U[instruction.register_1],
                instruction.register_2, core->U[instruction.register_2],
                instruction.register_3, core->U[instruction.register_3]);
    core->U[instruction.register_1] += core->U[instruction.register_2] * core->U[instruction.register_3];
    DEBUG_PRINT("--------Avant FMA--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n"
                "Le registre %d a pour valeur %ld\n",
                instruction.register_1, core->U[instruction.register_1],
                instruction.register_2, core->U[instruction.register_2],
                instruction.register_3, core->U[instruction.register_3]);
    core->IP += SIZE_INSTRUCTION_IN_BYTE;
}

void incu(core_t *core)
{
    instruction_t instruction = instruction_new(*(u32 *)&(core->file_buffer[core->IP]));
    DEBUG_PRINT("--------Avant Incrémentation--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n",
                instruction.register_1,
                core->U[instruction.register_1]);
    core->U[instruction.register_1]++;
    DEBUG_PRINT("--------Avant Incrémentation--------\n");
    DEBUG_PRINT("Le registre %d a pour valeur %ld\n\n",
                instruction.register_1,
                core->U[instruction.register_1]);

    core->IP += SIZE_INSTRUCTION_IN_BYTE;
}

void cmpu(core_t *core)
{
    instruction_t instruction = instruction_new(*(u32 *)&(core->file_buffer[core->IP]));

    u64 U0 = core->U[instruction.register_1];
    u64 U1 = core->U[instruction.register_2];
    DEBUG_PRINT("Le registre 1 a pour valeur: %ld\n"
                "Le registre 2 a pour valeur: %ld\n",
                U0, U1);

    DEBUG_PRINT("--------Avant Comparaison--------\n");
    for (int i = 7; i > 0; i--)
        DEBUG_PRINT("Compare Flag %d a pour valeur %d\n", i, core->CF[i]);
    core->CF[7] = (U0 == U1);
    core->CF[6] = (U0 != U1);
    core->CF[5] = (U0 > U1);
    core->CF[4] = (U0 >= U1);
    core->CF[3] = (U0 < U1);
    core->CF[2] = (U0 <= U1);
    DEBUG_PRINT("--------Après Comparaison--------\n");
    for (int i = 7; i > 0; i--)
        DEBUG_PRINT("Compare Flag %d a pour valeur %d\n", i, core->CF[i]);
    DEBUG_PRINT("\n");
    core->IP += SIZE_INSTRUCTION_IN_BYTE;
}

void jl(core_t *core)
{
    instruction_t instruction = instruction_new(*(u32 *)&(core->file_buffer[core->IP]));
    core->IP += SIZE_INSTRUCTION_IN_BYTE;
    u64 immediate = get_immediate(core);
    core->IP = core->CF[3] ? immediate : core->IP;
}

void outu(core_t *core)
{
    instruction_t instruction = instruction_new(*(u32 *)&(core->file_buffer[core->IP]));
    printf("%ld\n", core->U[instruction.register_1]);
    core->IP += SIZE_INSTRUCTION_IN_BYTE;
}

void outb(core_t *core)
{
    instruction_t instruction = instruction_new(*(u32 *)&(core->file_buffer[core->IP]));
    u64 address = core->U[instruction.register_1];           // Get value in register
    u8 address_value = *(u8 *)&(core->file_buffer[address]); // Search in file_buffer memory

    int i = 0;
    while (address_value)
    {
        printf("%c", address_value);
        i++;
        address_value = *(u8 *)&(core->file_buffer[address + i]);
    }
    core->IP += SIZE_INSTRUCTION_IN_BYTE;
}

void hlt(core_t *core)
{
    info("Core %d successful finished\n", core->id);
    core->IP += SIZE_INSTRUCTION_IN_BYTE;

    // exit(0); // à enlever pour parallélisme, sinon chut down dès un truc fini
}

void undefined_instruction(core_t *core)
{
    warn("Instruction Set don't defined the following instruction: %d. (maybe yet)", *(u8 *)&(core->file_buffer[core->IP]));
    exit(0);
}

void core_execute(core_t *self)
{
    header_t header = *(header_t *)(self->file_buffer);

    header.header_size = htobe64(header.header_size);
    header.address_data = htobe64(header.address_data);
    header.size_data = htobe64(header.size_data);
    header.address_code = htobe64(header.address_code);
    header.size_code = htobe64(header.size_code);
    header.address_parallel_on = htobe64(header.address_parallel_on);
    header.size_parallel_on = htobe64(header.size_parallel_on);
    header.core_number = htobe64(header.core_number);
    header.size_total = htobe64(header.size_total);

    DEBUG_PRINT(
        "magic number: %lx\n"
        "header size: %lx\n"
        "address data: %lx\n"
        "size data: %lx\n"
        "address code: %lx\n"
        "size code: %lx\n"
        "address parallel on: %lx\n"
        "size parallel on: %lx\n"
        "core number: %lx\n"
        "size total: %lx\n\n",

        header.magic_number,
        header.header_size,
        header.address_data,        // Data section address
        header.size_data,           // Data section size
        header.address_code,        // Code section address
        header.size_code,           // Code section size
        header.address_parallel_on, // Parallel on section address
        header.size_parallel_on,
        header.core_number,
        header.size_total // Size total);
    );

    self->IP = header.address_code;

    while (self->IP < header.size_total)
    {
        u8 opcode = *(u8 *)&(self->file_buffer[self->IP]);
        instruction_set[opcode](self);
    }
}

void core_drop(core_t *self)
{
    if (NULL != self)
    {
        if (NULL != self->memory)
            free(self->memory);
        if (NULL != self->file_buffer)
            free(self->file_buffer);
        free(self);
    }
}

void set_up_instruction_set()
{
    for (u8 indice = 0; indice < MAX_INSTRUCTION_NUMBER - 1; indice++)
        instruction_set[indice] = undefined_instruction;
    if (MAX_INSTRUCTION_NUMBER != 256)
        warn("Consider changing the %s index type to another type to avoid potential issues.", "u8");
    instruction_set[0] = loadu;
    instruction_set[6] = storeu;
    instruction_set[13] = movu;
    instruction_set[19] = movui;
    instruction_set[31] = addu;
    instruction_set[33] = mulu;
    instruction_set[36] = fmau;
    instruction_set[39] = incu;

    instruction_set[74] = cmpu;
    instruction_set[80] = jl;
    instruction_set[84] = outu;

    instruction_set[88] = outb;
    instruction_set[90] = hlt;
}

void read_config(char *config_file_name, char **file_buffer_list, u16 *number_of_file)
{
    FILE *config_file;
    char config_file_line[256];
    char full_path[256];
    u16 i = 0;
    u16 number_of_thread = 0;

    config_file = fopen(config_file_name, "r");

    if (config_file == NULL)
    {
        error("The configuration file %s cannot be opened.\n", config_file_name);
        exit(EXIT_FAILURE);
    }

    if (fscanf(config_file, "%255[^\n]\n", config_file_line) != EOF)
    {
        number_of_thread = atoi(config_file_line);
        error("The configuration file start with %d thread, it need to be the following format : <int> <path1> <path2> ...\n", number_of_thread);
        // Important : déclenche l'erreur lorsqu'il ne s'agit pas un nombre
    }

    // Lire chaque config_file_line du config_file
    while ((fscanf(config_file, "%255[^\n]\n", config_file_line) != EOF) && (i < number_of_thread))
    {
        // printf("config_file_line: %s\n", config_file_line); // Afficher la config_file_line lue
        struct stat st;
        snprintf(full_path, sizeof(full_path), "%s", config_file_line);
        // printf("full path: %s\n", full_path);
        if (stat(full_path, &st) != 0)
        {
            error("File not found or error accessing file %s.\n", config_file_line);
            fclose(config_file);
            exit(EXIT_FAILURE);
        }
        FILE *file = fopen(config_file_line, "rb");
        uint64_t total_size = (uint64_t)st.st_size;

        u8 *buffer = (u8 *)malloc(total_size);
        if (buffer == NULL)
        {
            error("File not found or error accessing file %s.\n", config_file_line);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        if (fread(buffer, 1, total_size, file) != total_size)
        {
            fprintf(stderr, "Erreur lors du chargement de données dans le buffer.\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        file_buffer_list[i] = buffer;
        i++;
        fclose(file);
    }

    // Fermer le config_file
    fclose(config_file);
    *number_of_file = i;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Error: Incorrect number of arguments\n");
        fprintf(stderr, "Usage: %s <config>\n", argv[0]);
        return 1;
    }

    char *file_buffer_list[MAX_FILE_BUFFER_SIZE];
    u16 n;

    read_config(argv[1], file_buffer_list, &n);
    set_up_instruction_set();

    u16 i = 0;
    while (i < n && file_buffer_list[i] != NULL)
    {
        core_t *core = core_new(file_buffer_list[i], i);

        core_execute(core);

        core_drop(core);

        i++;
    }

    return 0;
}
