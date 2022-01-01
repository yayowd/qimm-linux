/* This file is part of qimm project.
 * qimm is a Situational Linux Desktop Based on Weston.
 * Copyright (C) 2021 The qimm Authors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "share.h"

char *
get_command_line(char *const argv[]) {
    char *str = NULL;

    /* check the path is not empty */
    char *path = argv[0];
    if (!path || strlen(path) == 0)
        goto final;

    size_t size = 0;
    FILE *fp = open_memstream(&str, &size);
    if (!fp)
        goto final;

    fprintf(fp, "%s", path);
    for (int i = 1; ; ++i) {
        char *arg = argv[i];
        if (!arg)
            break;
        fprintf(fp, " %s", arg);
    }

    fclose(fp);

final:
    if (!str)
        qimm_log("get command line from argv failed");

    return str;
}

void
free_command_line(char *argv[]) {
    char **pos = argv;
    while (*pos) {
        free(*pos);
        pos++;
    }
    free(argv);
}

void
empty_region(pixman_region32_t *region) {
    pixman_region32_fini(region);
    pixman_region32_init(region);
}

void
random_rgb(float rgb[3]) {
    FILE *fp = popen("cat /proc/sys/kernel/random/uuid | cksum", "r");
    if (fp) {
        char *line = NULL;
        size_t size = 0;
        getline(&line, &size, fp);
        pclose(fp);

        if (line) {
            char *end;
            long num = strtol(line, &end, 10);
            free(line);

            rgb[0] = num % 1000 / 1000.f;
            num /= 1000;
            rgb[1] = num % 1000 / 1000.f;
            num /= 1000;
            rgb[2] = num % 1000 / 1000.f;
            return;
        }
    }
    rgb[0] = rand() % 255 / 255.;
    rgb[1] = rand() % 255 / 255.;
    rgb[2] = rand() % 255 / 255.;
}
