/*
 * Copyright (c) 2012 Natalia Portillo.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BLOCK_SIZE 65536

void show_usage() { printf("Usage: dd-diff <input_path> <output_path> [block_size]\n"); }

int main(int argc, char** argv)
{
    int     block_size = BLOCK_SIZE;
    ssize_t in_read, out_read, to_process, total_written = 0;
    int     fd_in, fd_out;
    char *  in_buf, *out_buf;
    int     i, dif;
    off_t   last_pos, current_pos;
    int progress_counter = 0;

    printf("dd-diff A DD that only writes changed blocks.\n");
    printf("Copyright (C) 2022 Natalia Portillo\n");

    if(argc != 3 && argc != 4)
    {
        show_usage();
        return 1;
    }

    if(argc == 4)
    {
        block_size = atoi(argv[3]);
        if(block_size == 0) block_size = BLOCK_SIZE;
    }

    in_buf = malloc(block_size);

    if(in_buf == NULL)
    {
        printf("Error: could not allocate memory.\n");
        return 1;
    }

    out_buf = malloc(block_size);

    if(out_buf == NULL)
    {
        printf("Error: could not allocate memory.\n");
        free(out_buf);
        return 1;
    }

    fd_in = open(argv[1], O_RDONLY);

    if(fd_in < 0)
    {
        printf("Error opening input file.\n");
        free(in_buf);
        free(out_buf);
        return 1;
    }

    fd_out = open(argv[2], O_RDWR);

    if(fd_out < 0)
    {
        printf("Error opening output file.\n");
        free(in_buf);
        free(out_buf);
        return 1;
    }

    last_pos = lseek(fd_in, 0, SEEK_END);
    lseek(fd_in, 0, SEEK_SET);

    in_read = read(fd_in, in_buf, block_size);

    printf("\rProcessing position %d of %ld (%f%%)",
           0,
           last_pos,
           0.0);

    while(in_read > 0)
    {
        progress_counter++;
        current_pos = lseek(fd_in, 0, SEEK_CUR);

        if(progress_counter >= 25)
        {
            printf("\rProcessing position %ld of %ld (%f%%)",
                   current_pos,
                   last_pos,
                   ((float)current_pos * 100.0) / (float)last_pos);
            progress_counter = 0;
        }

        out_read = read(fd_out, out_buf, block_size);

        to_process = in_read;
        if(out_read < to_process) to_process = out_read;

        dif = false;
        for(i = 0; i < to_process; i++)
        {
            if(in_buf[i] != out_buf[i])
            {
                dif = true;
                break;
            }
        }

        if(dif)
        {
            lseek(fd_in, -in_read, SEEK_CUR);
            lseek(fd_out, -out_read, SEEK_CUR);
            total_written += write(fd_out, in_buf, to_process);
        }

        in_read = read(fd_in, in_buf, block_size);
    }

    printf("\rProcessing position %ld of %ld (%f%%)",
           current_pos,
           last_pos,
           ((float)current_pos * 100.0) / (float)last_pos);

    close(fd_in);
    close(fd_out);
    free(in_buf);
    free(out_buf);

    return 0;
}
