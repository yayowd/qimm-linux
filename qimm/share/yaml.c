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

static int keep_in_place = 0;

static void
qimm_yaml_next_keep_in_place() {
    keep_in_place = 1;
}

int
qimm_yaml_next_event(yaml_parser_t *parser, yaml_event_t *event) {
    if (keep_in_place) {
//        qimm_log("qimm yaml next event: [%lu:%lu] keep in place type(%d)",
//                event->start_mark.line + 1,
//                event->start_mark.column,
//                event->type);
        keep_in_place = 0;
    } else {
        if (event->type != YAML_NO_EVENT)
            yaml_event_delete(event);

        if (!yaml_parser_parse(parser, event)) {
            qimm_log("qimm yaml next event error: [%lu:%lu] (%d)%s",
                     parser->problem_mark.line + 1,
                     parser->problem_mark.column,
                     parser->error,
                     parser->problem);
            return -1;
        }
    }
//    qimm_log("type: %d, buf(%s)\n"
//             "start:\tline: %zu,\tcolumn: %zu,\tindex: %zu\n"
//             "end:\tline: %zu,\tcolumn: %zu,\tindex: %zu",
//            event->type,
//            event->type == YAML_SCALAR_EVENT ? event->data.scalar.value : "",
//            event->start_mark.line + 1,
//            event->start_mark.column,
//            event->start_mark.index,
//            event->end_mark.line + 1,
//            event->end_mark.column,
//            event->end_mark.index
//    );
    return 0;
}

int
qimm_yaml_next_event_is(yaml_parser_t *parser, yaml_event_t *event,
                        yaml_event_type_t type, const char *error) {
    if (qimm_yaml_next_event(parser, event) < 0)
        return -1; // parse error
    if (event->type == type)
        return 0; // match type
    qimm_plog(error, "([%lu:%lu] type %d)",
              event->start_mark.line + 1,
              event->start_mark.column,
              event->type);
    return -2; // not match
}

char *
qimm_yaml_next_value(yaml_parser_t *parser, yaml_event_t *event) {
    if (qimm_yaml_next_event_is(parser, event,
                                YAML_SCALAR_EVENT,
                                "qimm yaml next value error: not value") < 0)
        return NULL;

    char *buf = (char *) event->data.scalar.value;
    return strdup(buf);
}

int
qimm_yaml_next_value_int(yaml_parser_t *parser, yaml_event_t *event) {
    char *value = qimm_yaml_next_value(parser, event);
    if (value) {
        int i = strtol(value, NULL, 0);
        free(value);
        return i;
    }
    return 0;
}

static int
qimm_yaml_next_skip(yaml_parser_t *parser, yaml_event_t *event) {
    if (qimm_yaml_next_event(parser, event) < 0)
        return -1;
    yaml_event_type_t start = event->type;
    yaml_event_type_t end;
    int deep = 1;
    switch (start) {
        case YAML_STREAM_START_EVENT:
        case YAML_DOCUMENT_START_EVENT:
        case YAML_SEQUENCE_START_EVENT:
        case YAML_MAPPING_START_EVENT:
            end = start + 1;
            break;
        default:
            goto succ;
    }

    do {
        if (qimm_yaml_next_event(parser, event) < 0)
            return -1;
        if (event->type == start)
            deep++;
        else if (event->type == end) {
            deep--;
            if (deep == 0)
                goto succ;
        }
    } while (event->type != YAML_STREAM_END_EVENT);
    return -1;

succ:
    qimm_log("qimm yaml next skip: [%lu:%lu] type(%d)",
             event->end_mark.line + 1,
             event->end_mark.column,
             event->type);
    return 0;
}

void *
qimm_yaml_read_mapping(yaml_parser_t *parser, yaml_event_t *event,
                       struct qimm_yaml_read_mapping_fun *fun) {
    if (qimm_yaml_next_event_is(parser, event,
                                YAML_MAPPING_START_EVENT,
                                "qimm yaml read mapping error: not object") < 0)
        return NULL;

    void *obj = fun->init();
    do {
        if (qimm_yaml_next_event(parser, event) < 0)
            goto err;

        if (event->type == YAML_MAPPING_END_EVENT)
            break;
        else if (event->type == YAML_SCALAR_EVENT) {
            char *key = (char *) event->data.scalar.value;
            int ret = fun->data(parser, event, obj, key);
            if (ret == -2) {
                qimm_log("qimm yaml read mapping warning: [%lu:%lu] unknow key(%s)",
                         event->start_mark.line + 1,
                         event->start_mark.column,
                         key);
                if (qimm_yaml_next_skip(parser, event) < 0) {
                    qimm_log("qimm yaml read mapping error: next skip");
                    goto err;
                }
            } else if (ret < 0)
                goto err;
        } else {
            qimm_log("qimm yaml read mapping error: [%lu:%lu] not key(type %d)",
                     event->start_mark.line + 1,
                     event->start_mark.column,
                     event->type);
            goto err;
        }
    } while (event->type != YAML_STREAM_END_EVENT);

    return obj;

err:
    fun->free(obj);
    return NULL;
}

int
qimm_yaml_read_sequence(yaml_parser_t *parser, yaml_event_t *event,
                        struct qimm_yaml_read_sequence_fun *fun) {
    if (qimm_yaml_next_event_is(parser, event,
                                YAML_SEQUENCE_START_EVENT,
                                "qimm yaml read sequence error: not list") < 0)
        return -1;

    do {
        if (qimm_yaml_next_event(parser, event) < 0)
            return -1;

        if (event->type == YAML_SEQUENCE_END_EVENT)
            break;

        qimm_yaml_next_keep_in_place();

        struct wl_list *item = fun->data(parser, event);
        if (item == NULL)
            return -1;
        wl_list_insert(fun->list->prev, item);
    } while (event->type != YAML_STREAM_END_EVENT);

    return 0;
}

int
qimm_yaml_write_document(const char *path,
                         qimm_yaml_write_data_func_t func,
                         void *data) {
    FILE *file;
    yaml_emitter_t emitter;
    yaml_event_t event;
    int ret = -1;

    file = fopen(path, "wb");
    if (file) {
        yaml_emitter_initialize(&emitter);
        yaml_emitter_set_output_file(&emitter, file);

        yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
        if (!yaml_emitter_emit(&emitter, &event)) goto err;

        yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 0);
        if (!yaml_emitter_emit(&emitter, &event)) goto err;

        if (func(&emitter, data) < 0) goto err;

        yaml_document_end_event_initialize(&event, 0);
        if (!yaml_emitter_emit(&emitter, &event)) goto err;

        yaml_stream_end_event_initialize(&event);
        if (!yaml_emitter_emit(&emitter, &event)) goto err;

        ret = 0;

err:
        yaml_emitter_delete(&emitter);
        fclose(file);
    }
    return ret;
}
