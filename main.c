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
#endif