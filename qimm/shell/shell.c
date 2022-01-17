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
terminate_binding(struct weston_keyboard *keyboard,
                  const struct timespec *time,
                  uint32_t key, void *data) {
    weston_compositor_exit(data);
}

static void
click_to_activate_binding(struct weston_pointer *pointer,
                          const struct timespec *time,
                          uint32_t button, void *data) {
    if (pointer->grab != &pointer->default_grab)
        return;
    if (pointer->focus == NULL)
        return;

    // activate_binding(pointer->seat, data, pointer->focus,
    //                  WESTON_ACTIVATE_FLAG_CLICKED |
    //                  WESTON_ACTIVATE_FLAG_CONFIGURE);
}

static void
touch_to_activate_binding(struct weston_touch *touch,
                          const struct timespec *time,
                          void *data) {
    if (touch->grab != &touch->default_grab)
        return;
    if (touch->focus == NULL)
        return;

    // activate_binding(touch->seat, data, touch->focus,
    //                  WESTON_ACTIVATE_FLAG_CONFIGURE);
}

static void
shell_add_bindings(struct weston_compositor *ec, struct qimm_shell *shell) {
    weston_compositor_add_key_binding(ec, KEY_BACKSPACE,
                                      MODIFIER_CTRL | MODIFIER_ALT,
                                      terminate_binding, ec);

    /* fixed bindings */
    weston_compositor_add_button_binding(ec, BTN_LEFT, 0,
                                         click_to_activate_binding, shell);
    weston_compositor_add_button_binding(ec, BTN_RIGHT, 0,
                                         click_to_activate_binding, shell);
    weston_compositor_add_touch_binding(ec, 0,
                                        touch_to_activate_binding, shell);
}

static void
qimm_shell_destroy(struct wl_listener *listener, void *data) {
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
                                                     &shell->destroy_listener,
                                                     qimm_shell_destroy)) {
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

    shell_add_bindings(ec, shell);

    clock_gettime(CLOCK_MONOTONIC, &shell->startup_time);
    return 0;

out:
    qimm_log("qimm shell failed to load, exiting...");
    return -1;
}
