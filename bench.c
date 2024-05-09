#include <math.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>

#include "vm.h"

#define MAX_SAMPLES 5
#define ITERATION 50
#define SIZE_FILE ((sizeof(u32) + sizeof(u64)) * 1000) // Parce qu'il y a pas de vecteur pour l'instant

typedef struct
{
    void (*opcode)(core_t *);
    u8 *name;
} Opcode;

//
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

    for (int i = 0; i < MAX_MEMORY_SIZE; ++i)
    {
        *(memory + i) = (u8)i;
    }

    core->memory = memory;

    // LES INTIALISER AVEC DES PLUS GRANDES VALEURS ???

    for (int i = 0; i < NUMBER_SCALAR_REGISTER; ++i)
    {
        core->U[i] = (rand() % (MAX_MEMORY_SIZE / 2)); // pour les déplacements mémoires
        core->S[i] = (i64)rand();
        core->F[i] = (f64)rand();
    }

    for (int i = 0; i < 8; i++)
    {
        core->CF[i] = false;
    }

    core->IP = 0;

    return core;
}

//
static u32 create_instruction(u8 opcode, u16 offset, u8 register_1, u8 register_2, u8 register_3)
{
    return htobe32(opcode << (SIZE_INSTRUCTION - 8) | offset << (SIZE_INSTRUCTION - 17) | register_1 << (SIZE_INSTRUCTION - 22) | register_2 << (SIZE_INSTRUCTION - 27) | register_3);
}

//
void sort_f64(f64 *restrict a, u64 n)
{
    for (u64 i = 0; i < n; i++)
        for (u64 j = i + 1; j < n; j++)
            if (a[i] > a[j])
            {
                f64 tmp = a[i];

                a[i] = a[j];
                a[j] = tmp;
            }
}

//
f64 mean_f64(f64 *restrict a, u64 n)
{
    f64 m = 0.0;

    for (u64 i = 0; i < n; i++)
        m += a[i];

    m /= (f64)n;

    return m;
}

//
f64 stddev_f64(f64 *restrict a, u64 n)
{
    f64 d = 0.0;
    f64 m = mean_f64(a, n);

    for (u64 i = 0; i < n; i++)
        d += (a[i] - m) * (a[i] - m);

    d /= (f64)(n - 1);

    return sqrt(d);
}

//
void mesure_performance_scalaire(void (*opcode)(core_t *), u64 r, const u8 *title)
{
    core_t *core = core_init();

    size_t size_file_buffer = (sizeof(u32) + sizeof(u64)) *r;
    u8 *file_buffer = (u8 *)malloc(size_file_buffer);

    if (file_buffer == NULL)
    {
        printf("failed to allocate file buffer of size %lu bytes", SIZE_FILE);
        core_drop(core);
        exit(1);
    }

    core->file_buffer = file_buffer;

    f64 elapsed = 0.0;
    struct timespec t1, t2;
    f64 samples[r];

    for (int i = 0; i < r; ++i)
    {
        u64 to_read = core->IP;
        // initialisation de l'instruction
        u8 r1 = rand() % 32;
        u8 r2 = rand() % 32;
        // u8 r2 = (temp == r1) ? (r1 + 1) % 32 : temp;
        u8 r3 = rand() % 32;
        u16 offset = 0; // pour l'instant parce que flemme de calculer

        //printf("r1: %u\tr2: %u\tr3: %u\tu_r1: %lu\tu_r2: %lu\tu_r3: %lu\n", r1, r2, r3, core->U[r1], core->U[r2], core->U[r3]);

        u32 *ptr_inst = (u32 *)(core->file_buffer + core->IP);
        u64 *ptr_imm = (u64 *)(core->file_buffer + core->IP + sizeof(u32));

        *ptr_inst = create_instruction(0, offset, r1, r2, r3);
        *ptr_imm = htobe64((rand() % size_file_buffer)); // pour les sauts d'adresse mémoire

        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
        for (int j = 0; j < ITERATION; ++j)
        {
            opcode(core);
            core->IP = to_read;
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &t2);

        elapsed = (f64)(t2.tv_nsec - t1.tv_nsec) / (f64)r;
        samples[i] = elapsed;
    }

    core_drop(core);

    sort_f64(samples, r);
    f64 min = samples[0];
    f64 max = samples[r - 1];
    f64 mean = mean_f64(samples, r);
    f64 dev = stddev_f64(samples, r);
    // f64 mbps = size_mib / (mean / 1e9);
    f64 size_b = 0;
    f64 mbps = 0;

    printf("%10s; %15.3lf; %10lu; %15.3lf; %15.3lf; %15.3lf; %15.3lf (%6.3lf %%); %10.3lf;\n",
           title,
           size_b,
           r,
           min,
           mean,
           max,
           dev,
           (dev * 100.0 / mean),
           mbps);

}

int main()
{
    Opcode opcode_tobench[] =
        {
            {loadu, "loadu"},
            {storeu, "storeu"},
            {movu, "movu"},
            {movui, "movui"},
            {addu, "addu"},
            {mulu, "mulu"},
            {fmau, "fmau"},
            {incu, "incu"},
            {cmpu, "cmpu"},
            {jl, "jl"},
        };

    // print header
    printf("%10s; %15s; %10s; %15s; %15s; %15s; %26s; %10s;\n",
           "opcode",
           "B",
           "r", "min (ns)", "mean (ns)", "max (ns)", "stddev (%)", "MiB/ns");

    u64 r = 1000;

    for (int i = 0; i < 10; ++i)
    {
        mesure_performance_scalaire(opcode_tobench[i].opcode, r, opcode_tobench[i].name);
    }

    return 0;
}