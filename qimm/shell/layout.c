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

static struct qimm_project_config_type *
qimm_layout_project_find_type_in(struct wl_list *types, const char *name) {
    struct qimm_project_config_type *type;
    wl_list_for_each(type, types, link) {
        if (!strcmp(type->name, name))
            return type;
    }
    return NULL;
}

static struct qimm_project_config_type *
qimm_layout_project_find_type(struct qimm_project *project, const char *name) {
    struct qimm_project_config_type *type = NULL;

    if (project->config)
        type = qimm_layout_project_find_type_in(&project->config->types, name);

    if (!type && project->shell->config) {
        struct wl_list *types = &project->shell->config->types;
        type = qimm_layout_project_find_type_in(types, name);
    }

    return type;
}

int
qimm_layout_project_init(struct qimm_project *project) {
    wl_list_init(&project->layouts);

    struct qimm_layout *layout;
    struct qimm_project_config_layout *config_layout;
    struct qimm_project_config_type *type;

    if (project->shell->config) {
        struct wl_list *layouts = &project->shell->config->layouts;
        wl_list_for_each(config_layout, layouts, link) {
            type = qimm_layout_project_find_type(project, config_layout->name);
            if (!type) {
                qimm_log("layout init error: project[%s] no matching type for layout[%s]",
                        project->name, config_layout->name);
                return -1;
            }

            layout = zalloc(sizeof *layout);
            layout->config_layout = config_layout;
            wl_list_insert(project->layouts.prev, &layout->link);
        }
    }

    wl_list_for_each(config_layout, &project->config->layouts, link) {
        type = qimm_layout_project_find_type(project, config_layout->name);
        if (!type) {
            qimm_log("layout init error: project[%s] no matching type for layout[%s]",
                    project->name, config_layout->name);
            return -1;
        }

        layout = zalloc(sizeof *layout);
        layout->config_layout = config_layout;
        wl_list_insert(project->layouts.prev, &layout->link);
    }

    return 0;
}

void
qimm_layout_project_clear(struct qimm_project *project) {
    struct qimm_layout *layout, *tmp;
    wl_list_for_each_safe(layout, tmp, &project->layouts, link) {
        if (layout->client)
            wl_client_destroy(layout->client->client);
        assert(layout->client == NULL);

        wl_list_remove(&layout->link);
        free(layout);
    }
}

struct qimm_layout *
qimm_layout_find_by_client(struct qimm_shell *shell, struct wl_client *client) {
    struct qimm_project *project = qimm_shell_get_current_project(shell);
    if (!project)
        return NULL;
    struct qimm_layout *layout;
    wl_list_for_each(layout, &project->layouts, link) {
        if (layout->client->client == client)
            return layout;
    }
    return NULL;
}

int
qimm_layout_project_update(struct qimm_project *project) {
    struct qimm_output *output = project->output;
    if (!output) {
        qimm_log("layout update error: project[%s] no output", project->name);
        return -1;
    }

    if (project->output_layouted != output) {
        int32_t w = output->output->width;
        int32_t h = output->output->height;

        /* pointer in current row */
        int32_t dx = 0; /* right */
        int32_t dy = 0, dy2 = 0; /* top and bottom */

        struct qimm_layout *layout;
        struct qimm_project_config_layout *cl;
        wl_list_for_each(layout, &project->layouts, link) {
            cl = layout->config_layout;

            layout->x = cl->x < 0 ? dx : cl->x;
            layout->y = cl->y < 0 ? dy : cl->y;
            layout->w = cl->w < 0 ? div(w, abs(cl->w)).quot : cl->w;
            layout->h = cl->h < 0 ? div(h, abs(cl->h)).quot : cl->h;

            dx = layout->x + layout->w;
            if (dx >= w) { /* out of screen */
                if (layout->x > 0) { /* move layout to next row */
                    dx = layout->w;
                    dy = dy2;
                    dy2 = dy + layout->h;
                    layout->x = 0;
                    layout->y = dy;
                } else { /* move pointer to next row */
                    dx = 0;
                    if (cl->y < 0)
                        dy = MAX(dy2, layout->y + layout->h);
                    else
                        dy = layout->y + layout->h;
                    dy2 = dy;
                }
            } else {
                dy = layout->y;
                dy2 = MAX(dy2, layout->y + layout->h);
            }
        }

        project->output_layouted = output;
    }
    return 0;
}
