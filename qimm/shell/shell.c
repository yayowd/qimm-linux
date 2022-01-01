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

extern const struct weston_desktop_api qimm_shell_desktop_api;

struct qimm_output *
qimm_shell_get_focus_output(struct qimm_shell *shell) {
    return shell->output_focus;
}

struct qimm_project *
qimm_shell_get_current_project(struct qimm_shell *shell) {
    if (shell->output_focus)
        return shell->output_focus->project_cur;
    return NULL;
}

static void
shell_destroy(struct wl_listener *listener, void *data) {
    struct qimm_shell *shell =
            container_of(listener, struct qimm_shell, destroy_listener);

    wl_list_remove(&shell->destroy_listener.link);

    qimm_project_unload(shell);
    qimm_client_release(shell);

    weston_desktop_destroy(shell->desktop);

    /* note: the projects in output has been cleared after project unload */
    qimm_output_release(shell);
    qimm_layer_release(shell);

    free(shell);
}

WL_EXPORT int
wet_shell_init(struct weston_compositor *ec, int *argc, char *argv[]) {
    qimm_log("qimm shell is loading");
    qimm_log("qimm shell run at %s", getenv("WAYLAND_DISPLAY"));

    struct qimm_shell *shell = zalloc(sizeof *shell);
    if (!shell)
        return -1;

    shell->compositor = ec;

    // handle destroy event
    if (!weston_compositor_add_destroy_listener_once(ec,
            &shell->destroy_listener, shell_destroy)) {
        free(shell);
        return -1;
    }

    qimm_layer_init(shell);
    qimm_output_init(shell);

    // create desktop environment
    shell->desktop = weston_desktop_create(ec, &qimm_shell_desktop_api, shell);
    if (!shell->desktop)
        goto out;

    if (qimm_client_init(shell) < 0)
        goto out;

    /* load projects at last */
    if (qimm_project_load(shell) < 0)
        goto out;

    clock_gettime(CLOCK_MONOTONIC, &shell->startup_time);
    return 0;

out:
    qimm_log("qimm shell failed to load, exiting...");
    shell_destroy(&shell->destroy_listener, NULL);
    return -1;
}
