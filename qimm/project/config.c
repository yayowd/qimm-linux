/* This file is part of qimm project.
 * qimm is a Situational Linux Desktop Based on Wayland.
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

/* --------- config: layouts --------- */
static void *
qimm_config_project_layout_init() {
    struct qimm_project_config_layout *layout = zalloc(sizeof *layout);
    return layout;
}

static int
qimm_config_project_layout_data(yaml_parser_t *parser, yaml_event_t *event,
        void *obj, char *key) {
    struct qimm_project_config_layout *layout = obj;
    if (!strcmp(key, "name"))
        layout->name = qimm_yaml_next_value(parser, event);
    else if (!strcmp(key, "x"))
        layout->x = qimm_yaml_next_value_int(parser, event);
    else if (!strcmp(key, "y"))
        layout->y = qimm_yaml_next_value_int(parser, event);
    else if (!strcmp(key, "w"))
        layout->w = qimm_yaml_next_value_int(parser, event);
    else if (!strcmp(key, "h"))
        layout->h = qimm_yaml_next_value_int(parser, event);
    else
        return -2;
    return 0;
}

static void
qimm_config_project_layout_free(void *obj) {
    struct qimm_project_config_layout *layout = obj;

    free(layout->name);
    free(layout);
}

static void
qimm_config_project_layout_print(struct qimm_project_config_layout *layout) {
    fprintf(stderr, "\tqimm_project_config_layout:\n"
                    "\t\tname: %s\n"
                    "\t\tx: %5d,  y: %5d\n"
                    "\t\tw: %5d,  h: %5d\n",
            layout->name, layout->x, layout->y, layout->w, layout->h);
}

static struct wl_list *
qimm_config_project_layouts_data(yaml_parser_t *parser, yaml_event_t *event) {
    static struct qimm_yaml_read_mapping_fun fun = {
            qimm_config_project_layout_init,
            qimm_config_project_layout_data,
            qimm_config_project_layout_free,
    };
    struct qimm_project_config_layout *layout =
            qimm_yaml_read_mapping(parser, event, &fun);
    if (!layout)
        return NULL;
    return &layout->link;
}

/* --------- config: types --------- */
static void *
qimm_config_project_type_init() {
    struct qimm_project_config_type *type = zalloc(sizeof *type);
    return type;
}

static int
qimm_config_project_type_data(yaml_parser_t *parser, yaml_event_t *event,
        void *obj, char *key) {
    struct qimm_project_config_type *type = obj;
    if (!strcmp(key, "name"))
        type->name = qimm_yaml_next_value(parser, event);
    else if (!strcmp(key, "summary"))
        type->summary = qimm_yaml_next_value(parser, event);
    else
        return -2;
    return 0;
}

static void
qimm_config_project_type_free(void *obj) {
    struct qimm_project_config_type *type = obj;

    free(type->name);
    free(type->summary);
    free(type);
}

static void
qimm_config_project_type_print(struct qimm_project_config_type *type) {
    fprintf(stderr, "\tqimm_project_config_type:\n"
                    "\t\tname: %s\n"
                    "\t\tsummary: %s\n",
            type->name, type->summary);
}

static struct wl_list *
qimm_config_project_types_data(yaml_parser_t *parser, yaml_event_t *event) {
    static struct qimm_yaml_read_mapping_fun fun = {
            qimm_config_project_type_init,
            qimm_config_project_type_data,
            qimm_config_project_type_free,
    };
    struct qimm_project_config_type *type =
            qimm_yaml_read_mapping(parser, event, &fun);
    if (!type)
        return NULL;
    return &type->link;
}

/* --------- config: themes --------- */
static void *
qimm_config_project_theme_init() {
    struct qimm_project_config_theme *theme = zalloc(sizeof *theme);
    return theme;
}

static int
qimm_config_project_theme_data(yaml_parser_t *parser, yaml_event_t *event,
        void *obj, char *key) {
    struct qimm_project_config_theme *theme = obj;
    if (!strcmp(key, "name"))
        theme->name = qimm_yaml_next_value(parser, event);
    else if (!strcmp(key, "foreground"))
        theme->foreground = qimm_yaml_next_value(parser, event);
    else if (!strcmp(key, "background"))
        theme->background = qimm_yaml_next_value(parser, event);
    else
        return -2;
    return 0;
}

static void
qimm_config_project_theme_free(void *obj) {
    struct qimm_project_config_theme *theme = obj;
    free(theme->name);
    free(theme->foreground);
    free(theme->background);
    free(theme);
}

static void
qimm_config_project_theme_print(struct qimm_project_config_theme *theme) {
    fprintf(stderr, "\tqimm_project_config_theme:\n"
                    "\t\tname: %s\n"
                    "\t\tforeground: %s\n"
                    "\t\tbackground: %s\n",
            theme->name, theme->foreground, theme->background);
}

static struct wl_list *
qimm_config_project_themes_data(yaml_parser_t *parser, yaml_event_t *event) {
    static struct qimm_yaml_read_mapping_fun fun = {
            qimm_config_project_theme_init,
            qimm_config_project_theme_data,
            qimm_config_project_theme_free,
    };
    struct qimm_project_config_theme *theme =
            qimm_yaml_read_mapping(parser, event, &fun);
    if (!theme)
        return NULL;
    return &theme->link;
}

/* --------- config: project --------- */
static void *
qimm_config_project_init() {
    struct qimm_project_config *config = zalloc(sizeof *config);
    wl_list_init(&config->themes);
    wl_list_init(&config->types);
    wl_list_init(&config->layouts);
    return config;
}

static int
qimm_config_project_data(yaml_parser_t *parser, yaml_event_t *event,
        void *obj, char *key) {
    struct qimm_project_config *config = obj;
    if (!strcmp(key, "name")) {
        config->name = qimm_yaml_next_value(parser, event);
    } else if (!strcmp(key, "themes")) {
        struct qimm_yaml_read_sequence_fun fun = {
                &config->themes,
                qimm_config_project_themes_data,
        };
        return qimm_yaml_read_sequence(parser, event, &fun);
    } else if (!strcmp(key, "types")) {
        struct qimm_yaml_read_sequence_fun fun = {
                &config->types,
                qimm_config_project_types_data,
        };
        return qimm_yaml_read_sequence(parser, event, &fun);
    } else if (!strcmp(key, "layouts")) {
        struct qimm_yaml_read_sequence_fun fun = {
                &config->layouts,
                qimm_config_project_layouts_data,
        };
        return qimm_yaml_read_sequence(parser, event, &fun);
    } else
        return -2;
    return 0;
}

void
qimm_config_project_free(void *project_config) {
    struct qimm_project_config *config = project_config;
    assert(config);

    struct qimm_project_config_theme *theme, *tmp;
    wl_list_for_each_safe(theme, tmp, &config->themes, link) {
        qimm_config_project_theme_free(theme);
    }

    struct qimm_project_config_type *type, *tmp2;
    wl_list_for_each_safe(type, tmp2, &config->types, link) {
        qimm_config_project_type_free(type);
    }

    struct qimm_project_config_layout *layout, *tmp3;
    wl_list_for_each_safe(layout, tmp3, &config->layouts, link) {
        qimm_config_project_layout_free(layout);
    }

    free(config->name);
    free(config);
}

static void
qimm_config_project_print(struct qimm_project_config *config) {
    fprintf(stderr, "qimm_project_config:\n"
                    "\tname: %s\n",
            config->name);

    struct qimm_project_config_theme *theme;
    wl_list_for_each(theme, &config->themes, link) {
        qimm_config_project_theme_print(theme);
    }

    struct qimm_project_config_type *type;
    wl_list_for_each(type, &config->types, link) {
        qimm_config_project_type_print(type);
    }

    struct qimm_project_config_layout *layout;
    wl_list_for_each(layout, &config->layouts, link) {
        qimm_config_project_layout_print(layout);
    }
}

/* --------- config: load --------- */
static char *
qimm_config_project_get_path(const char *name) {
    assert(name);

    char file[strlen(name) + 6];
    size_t len = snprintf(file, sizeof file, "%s.yaml", name);
    if (len >= sizeof file)
        return NULL;

    return qimm_get_project_path(file);
}

void *
qimm_config_project_load(const char *name) {
    char *path = qimm_config_project_get_path(name);
    if (!path)
        return NULL;
    qimm_log("project[%s] config with %s", name, path);

    FILE *fh = fopen(path, "r");
    if (!fh) {
        qimm_log("project[%s] failed to open config file", name);
        free(path);
        return NULL;
    }

    void *ret = NULL;
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        qimm_log("project[%s] failed to initialize yaml parser", name);
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
                    qimm_config_project_init,
                    qimm_config_project_data,
                    qimm_config_project_free,
            };
            ret = qimm_yaml_read_mapping(&parser, &event, &fun);
            if (!ret)
                goto err_event;
            qimm_config_project_print(ret);
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
