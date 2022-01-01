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

/** NOTE:
 *
 * copy from compositor/main.c: wet_get_binary_path
 * identical
 */
static char *
qimm_get_binary_path(const char *name, const char *dir) {
    char path[PATH_MAX];
    size_t len;

    len = weston_module_path_from_env(name, path, sizeof path);
    if (len > 0)
        return strdup(path);

    len = snprintf(path, sizeof path, "%s/%s", dir, name);
    if (len >= sizeof path)
        return NULL;

    return strdup(path);
}

char *
qimm_get_module_path(const char *name) {
    return qimm_get_binary_path(name, QIMM_MODULEDIR);
}

char *
qimm_get_project_path(const char *name) {
    return qimm_get_binary_path(name, QIMM_PROJECTDIR);
}
