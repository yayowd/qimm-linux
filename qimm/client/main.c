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

int
main(int argc, char **argv) {
    char *tmp = get_command_line(argv);
    printf("haha: client: %s\n", tmp);
    free(tmp);

    return 0;
}
