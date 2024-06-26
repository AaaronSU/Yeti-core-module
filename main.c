#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <stddef.h>

#include <string.h>
#include <libgen.h>
#include <errno.h>

#include <math.h>
#include <time.h>

#include "vm.h"

#ifndef CMOCKA_H_

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Error: Incorrect number of arguments\n");
        fprintf(stderr, "Usage: %s <config>\n", argv[0]);
        return 1;
    }
    u16 n;
    read_config(argv[1], file_buffer_list, &n);
    cpu_set_t cpuset;
    program_thread_data_t *tds = malloc(sizeof(program_thread_data_t) * n);
    set_up_instruction_set();
    CPU_ZERO(&cpuset);

    for (u16 i = 0; i < n; i++)
    {
        CPU_SET(i, &cpuset);
        tds[i].index = i;

        pthread_create(&tds[i].tid, NULL, execute_program_thread, &tds[i]);
        pthread_setaffinity_np(tds[i].tid, sizeof(cpuset), &cpuset);
    }

    for (u16 i = 0; i < n; i++)
    {
        pthread_join(tds[i].tid, NULL);
        printf("The thread %2d successful executed at core_id: %2d on numa_id: %d\n", tds[i].index, tds[i].core_id, tds[i].numa_id);
    }

    return 0;
}
#endif