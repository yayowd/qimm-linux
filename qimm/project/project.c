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

/*
 * the name of specific project
 */
#define QIMM_PROJECT_NAME_DASHBOARD "dashboard"
#define QIMM_PROJECT_NAME_ASSISTANT "assistant"

struct qimm_project *
qimm_project_create(struct qimm_shell *shell, const char *name) {
    assert(name);

    struct qimm_project *project = zalloc(sizeof *project);
    if (!project)
        return NULL;

    wl_list_init(&project->link);
    project->shell = shell;
    project->name = strdup(name);
    weston_layer_init(&project->layer, shell->compositor);

    project->config = qimm_config_project_load(project->name);
    if (!project->config) {
        qimm_log("failed to config project[%s]", project->name);
        goto err;
    }

    if (qimm_layout_project_init(project) < 0) {
        qimm_log("failed to init layout for project[%s]", project->name);
        goto err;
    }

    return project;

err:
    qimm_project_destroy(project);
    return NULL;
}

struct qimm_project *
qimm_project_create_dashboard(struct qimm_shell *shell) {
    struct qimm_project *project =
            qimm_project_create(shell, QIMM_PROJECT_NAME_DASHBOARD);

    return project;
}

struct qimm_project *
qimm_project_create_assistant(struct qimm_shell *shell) {
    struct qimm_project *project =
            qimm_project_create(shell, QIMM_PROJECT_NAME_ASSISTANT);

    return project;
}

void
qimm_project_destroy(struct qimm_project *project) {
    qimm_layout_project_clear(project);

    if (project->config)
        qimm_config_project_free(project->config);

    weston_layer_fini(&project->layer);
    free(project->name);
    free(project);
}

int
qimm_project_load(struct qimm_shell *shell) {
    // load project from cache directory
    if (qimm_project_load_from_disk(shell) < 0)
        return -1;

    /*
     * when project list is empty in output
     * add dashboard project to first output
     * add assistant project to other output
     */
    struct qimm_output *output;
    wl_list_for_each(output, &shell->outputs, link) {
        // add dashboard if empty
        if (wl_list_empty(&output->projects)) {
            struct qimm_project *project =
                    shell->outputs.next == &output->link ?
                    qimm_project_create_dashboard(shell) :
                    qimm_project_create_assistant(shell);
            if (project == NULL)
                return -1;

            qimm_output_project_insert(output, NULL, project);
        }

        // make sure first project is shown in each output
        struct qimm_project *first = container_of(output->projects.next,
                struct qimm_project, link);
        qimm_project_show(first);
    }

    return 0;
}

int
qimm_project_load_from_disk(struct qimm_shell *shell) {
    // TODO - yayowd 2021/8/21 - get cache directory and load data to project

    return 0;
}

void
qimm_project_unload(struct qimm_shell *shell) {
    struct qimm_output *output;
    wl_list_for_each(output, &shell->outputs, link) {
        struct qimm_project *project, *tmp;
        wl_list_for_each_safe(project, tmp, &output->projects, link) {
            qimm_output_project_remove(project);
            qimm_project_destroy(project);
        }
    }
}

void
qimm_project_show(struct qimm_project *project) {
    struct qimm_output *output = project->output;
    if (!output) {
        qimm_log("show project failed: project not in output");
        return;
    }

    if (output->project_cur) {
        if (output->project_cur == project)
            return;

        weston_layer_unset_position(&output->project_cur->layer);
    }

    output->project_cur = project;
    weston_layer_set_position(&project->layer, WESTON_LAYER_POSITION_NORMAL);

    qimm_layout_project_update(project);

    qimm_client_project_start(project);
}
