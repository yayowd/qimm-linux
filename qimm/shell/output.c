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

static void
qimm_output_destroy(struct qimm_output *qimm_output) {
    assert(wl_list_empty(&qimm_output->projects));
    assert(qimm_output->project_cur == NULL);

    wl_list_remove(&qimm_output->link);
    free(qimm_output);
}

static void
handle_output_destroy(struct wl_listener *listener, void *data) {
    struct qimm_output *qimm_output =
            container_of(listener, struct qimm_output, destroy_listener);

    wl_list_remove(&qimm_output->destroy_listener.link);

    /*
     * If the output is destroyed at runtime, move its projects to prev output.
     */
    if (qimm_output_project_move(qimm_output) < 0)
        return;

    qimm_output_destroy(qimm_output);
}

static struct qimm_output *
create_qimm_output(struct qimm_shell *shell, struct weston_output *output) {
    struct qimm_output *qimm_output = zalloc(sizeof *qimm_output);
    if (!qimm_output)
        return NULL;

    qimm_output->shell = shell;
    qimm_output->output = output;

    qimm_output->destroy_listener.notify = handle_output_destroy;
    wl_signal_add(&output->destroy_signal, &qimm_output->destroy_listener);

    wl_list_init(&qimm_output->projects);

    wl_list_insert(shell->outputs.prev, &qimm_output->link);
    if (!shell->output_focus)
        shell->output_focus = qimm_output;

    return qimm_output;
}

static void
handle_output_create(struct wl_listener *listener, void *data) {
    struct qimm_shell *shell =
            container_of(listener, struct qimm_shell, output_create_listener);
    struct weston_output *output = data;

    struct qimm_output *qimm_output = create_qimm_output(shell, output);

    /*
     * restore project from other output to this new output by output_name
     */
    qimm_output_project_restore(qimm_output);
}

static void
handle_output_move_layer(struct qimm_shell *shell,
        struct weston_layer *layer, void *data) {
    struct weston_output *output = data;
    struct weston_view *view;
    float x, y;

    wl_list_for_each(view, &layer->view_list.link, layer_link.link) {
        if (view->output != output)
            continue;

        x = view->geometry.x + output->move_x;
        y = view->geometry.y + output->move_y;
        weston_view_set_position(view, x, y);
    }
}

static void
handle_output_move(struct wl_listener *listener, void *data) {
    struct qimm_shell *shell = container_of(listener,
            struct qimm_shell, output_move_listener);

    qimm_layer_for_each(shell, handle_output_move_layer, data);
}

void
qimm_output_init(struct qimm_shell *shell) {
    wl_list_init(&shell->outputs);

    struct weston_output *output;
    wl_list_for_each(output, &shell->compositor->output_list, link) {
        create_qimm_output(shell, output);
    }

    shell->output_create_listener.notify = handle_output_create;
    wl_signal_add(&shell->compositor->output_created_signal,
            &shell->output_create_listener);

    shell->output_move_listener.notify = handle_output_move;
    wl_signal_add(&shell->compositor->output_moved_signal,
            &shell->output_move_listener);
}

void
qimm_output_release(struct qimm_shell *shell) {
    struct qimm_output *output, *tmp;
    wl_list_for_each_safe(output, tmp, &shell->outputs, link) {
        qimm_output_destroy(output);
    }

    wl_list_remove(&shell->output_create_listener.link);
    wl_list_remove(&shell->output_move_listener.link);
}

void
qimm_output_project_insert(struct qimm_output *output,
        struct wl_list *pos, struct qimm_project *project) {
    wl_list_remove(&project->link);
    wl_list_insert(pos ?: output->projects.prev, &project->link);

    project->output = output;
    free(project->output_name);
    project->output_name = strdup(output->output->name);
}

void
qimm_output_project_remove(struct qimm_project *project) {
    wl_list_remove(&project->link);

    if (project->output->project_cur == project)
        project->output->project_cur = NULL;

    project->output = NULL;
    free(project->output_name);
    project->output_name = NULL;
}

int
qimm_output_project_move(struct qimm_output *from) {
    /* no more peojects to move */
    if (wl_list_empty(&from->projects))
        return 0;

    /* no more output to move */
    if (wl_list_length(&from->shell->outputs) <= 1) {
        qimm_log("move project from output failed: no more output");
        return -1;
    }

    /* move projects to prev output */
    struct qimm_output *to = container_of(from->link.prev,
            struct qimm_output, link);
    struct qimm_project *project;
    wl_list_for_each(project, &from->projects, link) {
        wl_list_remove(&project->link);
        wl_list_insert(to->projects.prev, &project->link);

        project->output = to;
        /*
         * do not set
         *   output_name = to.output.name
         * because we should restore project to output
         * when output is recreated
         */
    }
    from->project_cur = NULL;
    return 0;
}

void
qimm_output_project_restore(struct qimm_output *to) {
    struct qimm_output *output;
    wl_list_for_each(output, &to->shell->outputs, link) {
        if (output == to)
            continue;

        /*
         * remember the insert pos, old output's project and
         * new project to shown when current old output's project
         * need be restored
         */
        struct wl_list *pos_old = NULL;
        struct qimm_project *project_old, *project_cur = NULL;

        struct qimm_project *project;
        wl_list_for_each(project, &output->projects, link) {
            if (!strcmp(project->output_name, to->output->name)) {
                /*
                 * when old output's project is showing in new output, whether
                 * to restore depends on whether a new project to shown
                 */
                if (output->project_cur == project) {
                    pos_old = to->projects.prev;
                    project_old = project;
                } else
                    qimm_output_project_insert(to, NULL, project);
            } else if (!pos_old || !project_cur)
                /*
                 * the new project to shown
                 * is the last before current old output's project
                 * or the first after current old output's project
                 */
                project_cur = project;
        }

        /* when need to restore current old output's project */
        if (pos_old && project_cur) {
            qimm_output_project_insert(to, pos_old, project_old);

            /* show new project in this output */
            qimm_project_show(project_cur);
        }
    }
}
