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
#include "qimm-desktop-shell-server-protocol.h"

static void
desktop_shell_test(struct wl_client *client,
                   struct wl_resource *resource) {
    qimm_log("desktop_shell_set_background ...");
}

static const struct qimm_desktop_shell_interface desktop_shell_implementation = {
        desktop_shell_test,
};

static void
unbind_desktop_shell(struct wl_resource *resource) {
    // struct qimm_shell *shell = wl_resource_get_user_data(resource);

//    if (shell->locked)
//        resume_desktop(shell);

    // shell->child.desktop_shell = NULL;
//    shell->prepare_event_sent = false;
}

static void
bind_desktop_shell(struct wl_client *client,
                   void *data, uint32_t version, uint32_t id) {
    struct qimm_shell *shell = data;
    struct wl_resource *resource;

    resource = wl_resource_create(client, &qimm_desktop_shell_interface,
                                  1, id);

//    if (client == shell->child.client) {
    wl_resource_set_implementation(resource,
                                   &desktop_shell_implementation,
                                   shell, unbind_desktop_shell);
    // shell->child.desktop_shell = resource;
//        return;
//    }
//
//    wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
//            "permission to bind desktop_shell denied");
}

int
qimm_client_init(struct qimm_shell *shell) {
    shell->client_path = qimm_get_module_path(QIMM_SHELL_CLIENT);
    if (shell->client_path == NULL)
        return -1;

    shell->global_shell_iface = wl_global_create(shell->compositor->wl_display,
                                                 &qimm_desktop_shell_interface,
                                                 1,
                                                 shell, bind_desktop_shell);
    if (!shell->global_shell_iface) {
        qimm_log("failed to create global shell interface");
        return -1;
    }

    return 0;
}

void
qimm_client_release(struct qimm_shell *shell) {
    free(shell->client_path);

    if (shell->global_shell_iface)
        wl_global_destroy(shell->global_shell_iface);
}

static void
destroy_shell_client_process(struct wl_listener *listener, void *data) {
    struct qimm_client *qimm_client =
            container_of(listener, struct qimm_client, destroy_listener);

    wl_list_remove(&qimm_client->destroy_listener.link);

    if (qimm_client->resource)
        wl_resource_destroy(qimm_client->resource);

    if (qimm_client->layout)
        qimm_client->layout->client = NULL;

    free(qimm_client);
}

static void
qimm_client_launch_process(void *data) {
    struct qimm_client_startup *startup = data;
    struct qimm_shell *shell = startup->shell;
    struct qimm_client *qimm_client = NULL;

    struct wl_client *client =
            qimm_process_launch(shell->compositor, startup->agrv);
    if (client) {
        qimm_client = zalloc(sizeof *qimm_client);
        qimm_client->client = client;
        qimm_client->destroy_listener.notify = destroy_shell_client_process;
        wl_client_add_destroy_listener(client, &qimm_client->destroy_listener);
    }

    startup->func(shell, qimm_client, startup->data);

    free_command_line(startup->agrv);
    free(startup);
}

void
qimm_client_start(struct qimm_client_startup *startup) {
    assert(startup->func);

    struct wl_display *display = startup->shell->compositor->wl_display;
    struct wl_event_loop *loop = wl_display_get_event_loop(display);
    if (loop)
        if (wl_event_loop_add_idle(loop, qimm_client_launch_process, startup))
            return;

    startup->func(startup->shell, NULL, startup->data);

    free_command_line(startup->agrv);
    free(startup);
}

static void
qimm_client_start_layout_func(struct qimm_shell *shell,
                              struct qimm_client *client, void *data) {
    if (client) { /* success launch client */
        struct qimm_layout *layout = data;

        layout->client = client;
        client->layout = layout;
    }
}

static void
qimm_client_start_layout(struct qimm_project *project, struct qimm_layout *layout) {
    struct qimm_client_startup *startup = zalloc(sizeof *startup);
    startup->shell = project->shell;
    startup->agrv = zalloc(6 * sizeof(char *));
    startup->agrv[0] = strdup(project->shell->client_path);
    startup->agrv[1] = strdup("-p");
    startup->agrv[2] = strdup(project->name);
    startup->agrv[3] = strdup("-a");
    startup->agrv[4] = strdup(layout->config_layout->name);
    startup->func = qimm_client_start_layout_func;
    startup->data = layout;
    qimm_client_start(startup);
}

void
qimm_client_project_start(struct qimm_project *project) {
    struct qimm_output *output = project->output;
    if (!output) {
        qimm_log("client start error: project (%s) no output", project->name);
        return;
    }

    struct qimm_layout *layout;
    wl_list_for_each(layout, &project->layouts, link) {
        if (!layout->client)
            qimm_client_start_layout(project, layout);
    }
}
