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

/*
 * the name of specific project
 */
#define QIMM_PROJECT_NAME_DASHBOARD "dashboard"
#define QIMM_PROJECT_NAME_ASSISTANT "assistant"

static int
qimm_project_setup(struct qimm_shell *shell,
                   const char *name,
                   const char *config_name,
                   struct qimm_project *project) {
    wl_list_init(&project->link);
    wl_list_init(&project->layouts);
    weston_layer_init(&project->layer, shell->compositor);

    project->shell = shell;
    project->name = strdup(name);

    if (config_name)
        project->config_name = strdup(config_name);
    if (project->config_name) {
        project->config = qimm_config_project_load(project->name,
                                                   project->config_name);
        if (!project->config) {
            qimm_log("project (%s) config failed", project->name);
            goto err;
        }
    }

    if (qimm_layout_project_init(project) < 0) {
        qimm_log("project (%s) layout init failed", project->name);
        goto err;
    }

    return 0;

err:
    qimm_project_destroy(project);
    return -1;
}

struct qimm_project *
qimm_project_create(struct qimm_shell *shell,
                    const char *name,
                    const char *config_name) {
    assert(name);

    struct qimm_project *project = zalloc(sizeof *project);
    if (!project)
        return NULL;
    qimm_log("project (%s) is new to created", name);

    if (qimm_project_setup(shell, name, config_name, project) < 0)
        return NULL;

    return project;
}

struct qimm_project *
qimm_project_create_dashboard(struct qimm_shell *shell) {
    struct qimm_project *project;

    project = qimm_project_create(shell,
                                  QIMM_PROJECT_NAME_DASHBOARD,
                                  QIMM_PROJECT_NAME_DASHBOARD);
    return project;
}

struct qimm_project *
qimm_project_create_assistant(struct qimm_shell *shell) {
    struct qimm_project *project;

    project = qimm_project_create(shell,
                                  QIMM_PROJECT_NAME_ASSISTANT,
                                  QIMM_PROJECT_NAME_ASSISTANT);
    return project;
}

void
qimm_project_destroy(struct qimm_project *project) {
    qimm_layout_project_clear(project);

    if (project->config)
        qimm_config_project_free(project->config);

    weston_layer_fini(&project->layer);
    free(project->name);
    free(project->config_name);
    free(project);
}

static struct qimm_project *
qimm_project_read(struct qimm_shell *shell, const char *name) {
    assert(name);

    struct qimm_project *project = qimm_data_read_project(shell, name);
    if (!project)
        return NULL;

    if (qimm_project_setup(shell, name, NULL, project) < 0)
        return NULL;

    if (project->output_name)
        project->output = qimm_output_find_by_name(shell, project->output_name);
    if (!project->output)
        project->output = qimm_output_get_default(shell);
    wl_list_insert(project->output->projects.prev, &project->link);

    return project;
}

static int
qimm_project_load_from_disk(struct qimm_shell *shell) {
    int ret = 0;

    struct dirent *ent;
    DIR *dir = opendir(shell->data_path);
    if (dir) {
        while (ent = readdir(dir)) {
            if (ent->d_type == DT_DIR) {
                if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
                    continue;

                if (!qimm_project_read(shell, ent->d_name)) {
                    ret = -1;
                    break;
                }
            }
        }
        closedir(dir);
    }

    return ret;
}

static int
qimm_project_prepare_for_output(struct qimm_shell *shell) {
    /*
     * when project list is empty in output
     * add dashboard project to first output
     * add assistant project to other output
     */
    struct qimm_output *output;
    wl_list_for_each(output, &shell->outputs, link) {
        // add if empty
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
        struct qimm_project *first =
                container_of(output->projects.next, struct qimm_project, link);
        qimm_project_show(first);
    }

    return 0;
}

int
qimm_project_load(struct qimm_shell *shell) {
    /* check data directory */
    shell->data_path = qimm_data_path();
    if (!shell->data_path) {
        qimm_log("failed to get path for data directory");
        return -1;
    }

    /* load common config for all projects */
    shell->config = qimm_config_project_load("shell->common", "common");
    if (!shell->config) {
        qimm_log("project (%s) config failed", "common");
        return -1;
    }

    // load project from cache directory
    if (qimm_project_load_from_disk(shell) < 0)
        return -1;

    if (qimm_project_prepare_for_output(shell) < 0)
        return -1;

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

    if (shell->config)
        qimm_config_project_free(shell->config);

    if (shell->data_path)
        free(shell->data_path);
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
