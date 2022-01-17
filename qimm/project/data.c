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
#include "qimm.h"

#define DIR_MODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH |S_IXOTH)

char *
qimm_data_path(void) {
    char *path_config, *path_qimm = NULL;
    const char *home_dir = getenv("HOME");

    if (home_dir)
        if (asprintf(&path_config, "%s/.config", home_dir) > 0) {
            mkdir(path_config, DIR_MODE);
            if (asprintf(&path_qimm, "%s/%s", path_config, QIMM_NAME) > 0)
                mkdir(path_qimm, DIR_MODE);
            free(path_config);
        }

    if (path_qimm) {
        DIR *dir = opendir(path_qimm);
        if (dir) {
            closedir(dir);
            return path_qimm;
        }
        free(path_qimm);
    }
    return NULL;
}

/* --------- project --------- */
static char *
qimm_data_project_path(struct qimm_project *project) {
    char *path;

    if (asprintf(&path, "%s/%s",
                 project->shell->data_path,
                 project->name) > 0)
        return path;

    return NULL;
}

static int
qimm_data_save_project_func(yaml_emitter_t *emitter, void *data) {
    struct qimm_project *project = data;
    yaml_event_t event;

    /* map object strat */
    yaml_mapping_start_event_initialize(&event, NULL,
                                        (yaml_char_t *) YAML_MAP_TAG, 1,
                                        YAML_ANY_MAPPING_STYLE);
    if (!yaml_emitter_emit(emitter, &event)) goto err;

    /* config_name */
    if (project->config_name) {
        yaml_scalar_event_initialize(&event, NULL,
                                     (yaml_char_t *) YAML_STR_TAG,
                                     (yaml_char_t *) "config_name",
                                     strlen("config_name"),
                                     1, 0,
                                     YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event)) goto err;
        yaml_scalar_event_initialize(&event, NULL,
                                     (yaml_char_t *) YAML_STR_TAG,
                                     (yaml_char_t *) project->config_name,
                                     strlen(project->config_name),
                                     1, 0,
                                     YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event)) goto err;
    }

    /* output_name */
    if (project->output_name) {
        yaml_scalar_event_initialize(&event, NULL,
                                     (yaml_char_t *) YAML_STR_TAG,
                                     (yaml_char_t *) "output_name",
                                     strlen("output_name"),
                                     1, 0,
                                     YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event)) goto err;
        yaml_scalar_event_initialize(&event, NULL,
                                     (yaml_char_t *) YAML_STR_TAG,
                                     (yaml_char_t *) project->output_name,
                                     strlen(project->output_name),
                                     1, 0,
                                     YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event)) goto err;
    }

    /* map object end */
    yaml_mapping_end_event_initialize(&event);
    if (!yaml_emitter_emit(emitter, &event)) goto err;

    return 0;

err:
    return -1;
}

/*
 * make data directory for project
 * save project datas when it changed.
 */
int
qimm_data_save_project(struct qimm_project *project) {
    int ret = -1;
    char *path, *config = NULL;

    path = qimm_data_project_path(project);
    if (path) {
        mkdir(path, DIR_MODE);
        DIR *dir = opendir(path);
        if (dir) {
            closedir(dir);

            if (asprintf(&config, "%s/config.yaml", path) > 0)
                ret = qimm_yaml_write_document(config,
                                               qimm_data_save_project_func,
                                               project);
        }
    }

    free(path);
    free(config);
    return ret;
}

static void *
qimm_data_read_project_init() {
    struct qimm_project *project = zalloc(sizeof *project);
    return project;
}

static int
qimm_data_read_project_data(yaml_parser_t *parser, yaml_event_t *event,
                            void *obj, char *key) {
    struct qimm_project *project = obj;
    if (!strcmp(key, "config_name"))
        project->config_name = qimm_yaml_next_value(parser, event);
    else if (!strcmp(key, "output_name"))
        project->output_name = qimm_yaml_next_value(parser, event);
    else
        return -2;
    return 0;
}

static void
qimm_data_read_project_free(void *obj) {
    struct qimm_project *project = obj;
    free(project->config_name);
    free(project->output_name);
    free(project);
}

void *
qimm_data_read_project(struct qimm_shell *shell, const char *name) {
    char *path;

    if (asprintf(&path, "%s/%s/config.yaml", shell->data_path, name) < 0)
        return NULL;
    qimm_log("project (%s) read from %s", name, path);

    FILE *fh = fopen(path, "r");
    if (!fh) {
        qimm_log("project (%s) read failed: open file error", name);
        free(path);
        return NULL;
    }

    void *ret = NULL;
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        qimm_log("project (%s) read failed: initialize yaml parser error",
                 name);
        goto err;
    }
    yaml_parser_set_input_file(&parser, fh);

    yaml_event_t event;
    event.type = YAML_NO_EVENT; // mark is empty to auto delete in next event
    do {
        if (qimm_yaml_next_event(&parser, &event) < 0)
            goto err_parse;

        if (event.type == YAML_DOCUMENT_START_EVENT) {
            struct qimm_yaml_read_mapping_fun fun = {
                    qimm_data_read_project_init,
                    qimm_data_read_project_data,
                    qimm_data_read_project_free,
            };
            ret = qimm_yaml_read_mapping(&parser, &event, &fun);
            if (!ret)
                goto err_event;
        } else if (event.type == YAML_DOCUMENT_END_EVENT)
            break;
    } while (event.type != YAML_STREAM_END_EVENT);

err_event:
    yaml_event_delete(&event);

err_parse:
    yaml_parser_delete(&parser);

err:
    fclose(fh);
    free(path);
    return ret;
}

/* --------- background --------- */
static int
qimm_data_save_background_func(yaml_emitter_t *emitter, void *base) {
    struct qimm_data_background *d =
            container_of(base, struct qimm_data_background, base);

    yaml_event_t event;
    char *buf;
    int len;

    /* map object strat */
    yaml_mapping_start_event_initialize(&event, NULL,
                                        (yaml_char_t *) YAML_MAP_TAG, 1,
                                        YAML_ANY_MAPPING_STYLE);
    if (!yaml_emitter_emit(emitter, &event)) goto err;

    /* color */
    if (d->color != 0) {
        yaml_scalar_event_initialize(&event, NULL,
                                     (yaml_char_t *) YAML_STR_TAG,
                                     (yaml_char_t *) "color", 5,
                                     1, 0,
                                     YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event)) goto err;
        len = asprintf(&buf, "%d", d->color);
        if (len < 0) goto err;
        yaml_scalar_event_initialize(&event, NULL,
                                     (yaml_char_t *) YAML_INT_TAG,
                                     (yaml_char_t *) buf, len,
                                     1, 0,
                                     YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event)) goto err;
    }

    /* image */
    if (d->image) {
        yaml_scalar_event_initialize(&event, NULL,
                                     (yaml_char_t *) YAML_STR_TAG,
                                     (yaml_char_t *) "image", 5,
                                     1, 0,
                                     YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event)) goto err;
        yaml_scalar_event_initialize(&event, NULL,
                                     (yaml_char_t *) YAML_STR_TAG,
                                     (yaml_char_t *) d->image, strlen(d->image),
                                     1, 0,
                                     YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event)) goto err;
    }

    /* type */
    if (d->type) {
        yaml_scalar_event_initialize(&event, NULL,
                                     (yaml_char_t *) YAML_STR_TAG,
                                     (yaml_char_t *) "type", 4,
                                     1, 0,
                                     YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event)) goto err;
        yaml_scalar_event_initialize(&event, NULL,
                                     (yaml_char_t *) YAML_STR_TAG,
                                     (yaml_char_t *) d->type, strlen(d->type),
                                     1, 0,
                                     YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event)) goto err;
    }

    /* map object end */
    yaml_mapping_end_event_initialize(&event);
    if (!yaml_emitter_emit(emitter, &event)) goto err;

    return 0;

err:
    qimm_log("Failed save data: %s (%s)", "background", d->base.name);
    return -1;
}

int
qimm_data_save_background(struct qimm_project *project,
                          struct qimm_data_background *background) {
    int ret = -1;

    char *path = qimm_data_project_path(project);
    if (path) {
        char *file;
        if (asprintf(&file, "%s/%s.yaml", path, background->base.name) > 0) {
            ret = 0;

            free(file);
        }

        free(path);
    }

    return ret;
}
