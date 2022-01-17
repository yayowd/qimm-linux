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
#ifndef QIMM_SHARE_H
#define QIMM_SHARE_H

#include "config.h"
#include "qimm-config.h"
#ifdef DEBUG
#include "tests/test-config.h"
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <linux/limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <dirent.h>
#include <wayland-version.h>
#include <wayland-util.h>
#include <yaml.h>
#include <pixman.h>

#include "shared/helpers.h"
#include "shared/file-util.h"
#include "shared/os-compatibility.h"
#include "libweston/version.h"
#include "libweston/libweston.h"
#include "libweston-desktop/libweston-desktop.h"
#include "compositor/weston.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------- log --------- */
#ifdef QIMM_REL_SRC_DIR
// strip prefix from __FILE__, leaving the path relative to the project root
#define _QIMM_FILE ((const char *)__FILE__ + sizeof(QIMM_REL_SRC_DIR) - 1)
#else
#define _QIMM_FILE __FILE__
#endif

/* literal only */
#define qimm_log(fmt, ...) \
    weston_log("[qimm] [%s:%d] " fmt "\n", _QIMM_FILE, __LINE__, ##__VA_ARGS__)

/* literal & char * */
#define qimm_plog(m, f, ...) ({          \
    char *j;                             \
    int l = asprintf(&j, "[qimm] [%s:%d] %s %s\n", _QIMM_FILE, __LINE__, m, f); \
    if (l == -1) {                       \
        weston_log(f, ##__VA_ARGS__);    \
    } else {                             \
        weston_log(j, ##__VA_ARGS__);    \
        free(j);                         \
    } })

/* --------- file --------- */
char *
qimm_get_module_path(const char *name);
char *
qimm_get_project_path(const char *name);

/* --------- yaml --------- */
int
qimm_yaml_next_event(yaml_parser_t *parser, yaml_event_t *event);
int
qimm_yaml_next_event_is(yaml_parser_t *parser, yaml_event_t *event,
                        yaml_event_type_t type, const char *error);

char *
qimm_yaml_next_value(yaml_parser_t *parser, yaml_event_t *event);
int
qimm_yaml_next_value_int(yaml_parser_t *parser, yaml_event_t *event);

struct qimm_yaml_read_mapping_fun {
    /* initialize target struct object */
    void *(*init)();
    /*
     * fill data to object with key
     * return:
     *     0   success
     *    -1   error
     *    -2   unknow key
     */
    int (*data)(yaml_parser_t *parser, yaml_event_t *event,
                void *obj, char *key);
    /* free object when error occurred */
    void (*free)(void *obj);

};

void *
qimm_yaml_read_mapping(yaml_parser_t *parser, yaml_event_t *event,
                       struct qimm_yaml_read_mapping_fun *fun);

struct qimm_yaml_read_sequence_fun {
    /* list to fill */
    struct wl_list *list;
    /* make each item data and return link to it */
    struct wl_list *(*data)(yaml_parser_t *parser, yaml_event_t *event);
};

int
qimm_yaml_read_sequence(yaml_parser_t *parser, yaml_event_t *event,
                        struct qimm_yaml_read_sequence_fun *fun);

typedef int (*qimm_yaml_write_data_func_t)(yaml_emitter_t *emitter,
                                           void *data);
int
qimm_yaml_write_document(const char *path,
                         qimm_yaml_write_data_func_t func,
                         void *data);

/* --------- shares --------- */
char *
get_command_line(char *const argv[]);
void
free_command_line(char *argv[]);

void
empty_region(pixman_region32_t *region);

void
random_rgb(float rgb[3]);

#ifdef  __cplusplus
}
#endif

#endif // QIMM_SHARE_H
