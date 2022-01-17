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
#ifndef QIMM_H
#define QIMM_H

#include "share.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Qimm shell use multiple projects to composite a desktop.
 */
struct qimm_shell {
    struct wl_listener destroy_listener;

    struct weston_compositor *compositor;
    struct weston_desktop *desktop;

    struct wl_listener output_create_listener;
    struct wl_listener output_move_listener;
    struct wl_list outputs; /* qimm_output::link */
    struct qimm_output *output_focus;

    /* the layer for background in all outputs */
    struct weston_layer background_layer;

    /* the path of qimm client */
    char *client_path;
    /* the interface of global desktop shell */
    struct wl_global *global_shell_iface;

    /* the common config for all projects */
    struct qimm_project_config *config;

    /* path for data directory */
    char *data_path;

    bool locked;

    struct timespec startup_time;
};

/*
 * multiple and dynamic output support
 */
struct qimm_output {
    struct wl_list link; /* qimm_shell::outputs */

    struct qimm_shell *shell;
    struct weston_output *output;
    struct wl_listener destroy_listener;

    /*
     * many projects will attach to one output, each output can
     * only display one project at a time.
     */
    struct wl_list projects; /* qimm_project::link */
    struct qimm_project *project_cur; /* the current project on show */
};

/*
 * There is a project per screen in qimm desktop.
 * Each project is some work organized according to user preferences.
 *
 * The project on screen can be operated by user, e.g. input text,
 * show picture, open web page, run command, exec application, and so on.
 * project will save status after user operation.
 *
 * The project maintain all of its data by itself. So it can restore status
 * after reboot, and also can sync to other device.
 *
 * The project layout by qimm-shell, and can resize and reposition by user.
 * The project show data by qimm-client and third applications.
 *
 * When previous project is finished, user can create next project, this
 * reflects the actual work of the user.
 */
struct qimm_project {
    struct wl_list link; /* qimm_output::projects */
    struct qimm_output *output;
    char *output_name; /* restore projects when output recreated */

    struct qimm_shell *shell;
    char *name;
    struct weston_layer layer;

    /* the config for project */
    char *config_name;
    struct qimm_project_config *config;

    /*
     * the current client layout in project
     * need to be updated when output, client and layout config changed.
     */
    struct wl_list layouts; /* qimm_layout::link */
    struct qimm_output *output_layouted; /* last layout output */
};

/*
 * The common config in qimm_shell and project config in qimm_project.
 * Each config contain themes, types, and layouts for project render.
 *
 * Each layout render a type in specified postion at runtime.
 */
struct qimm_project_config {
    char *name;
    struct wl_list themes; /* qimm_project_config_theme::link */
    struct wl_list types; /* qimm_project_config_type::link */
    struct wl_list layouts; /* qimm_project_config_layout::link */
};
struct qimm_project_config_theme {
    struct wl_list link; /* qimm_project_config::themes */

    char *name;
    char *foreground;
    char *background;
};
struct qimm_project_config_type {
    struct wl_list link; /* qimm_project_config::types */

    char *name;
    char *summary;
};
struct qimm_project_config_layout {
    struct wl_list link; /* qimm_project_config::layouts */

    char *name;

    /* location in config */
    int32_t x, y;
    int32_t w, h;
};

/*
 * Each layout in project config layouts.
 * Associate to the client and provide location information.
 */
struct qimm_layout {
    struct wl_list link; /* qimm_project::layouts */

    struct qimm_project *project;
    struct qimm_project_config_layout *config_layout;

    /* location in runtime, update if necessray */
    int32_t x, y;
    int32_t w, h;

    /*
     * filled when client process started
     * cleared when client destroyed
     * used to find layout for client surface
     */
    struct qimm_client *client;
};

/*
 * The client to render layout for project
 */
struct qimm_client {
    struct wl_client *client;
    struct wl_resource *resource;
    struct wl_listener destroy_listener;

    struct weston_surface *surface;

    /*
     * filled when client process started
     * used to clear client field in layout when client destroyed
     */
    struct qimm_layout *layout;
};
/*
 * client start helper
 * client == NULL when error
 */
typedef void(*qimm_client_startup_func_t)(struct qimm_shell *shell,
                                          struct qimm_client *client,
                                          void *data);
struct qimm_client_startup {
    struct qimm_shell *shell;

    /* argv[0] is the path of execute file */
    char **agrv;

    qimm_client_startup_func_t func;
    void *data;
};

/*
 * qimm surface mange surfaces and views for project
 */
struct qimm_surface {
    struct wl_signal destroy_signal;

    struct weston_desktop_surface *desktop_surface;
    struct weston_view *view;

    struct wl_list children_list;
    struct wl_list children_link;

    struct qimm_layout *layout;
};

/*
 * The project data saved to user HOME dir at runtime.
 *
 * Use a separate directory to save each project.
 * Use a sepatate yaml file in project directory to save each project data.
 */
struct qimm_data_base {
    char *name;
    qimm_yaml_write_data_func_t func;
};
struct qimm_data_background {
    struct qimm_data_base base;

    uint32_t color;
    char *image;
    char *type;
    int type_e;
};
enum QIMM_DATA_BACKGROUND_TYPE {
    SCALE,
    CROP,
    TILE,
    CENTERED
};

/* --------- layer --------- */
void
qimm_layer_init(struct qimm_shell *shell);
void
qimm_layer_release(struct qimm_shell *shell);
/*
 * call func to each shown layer in shell
 */
typedef void (*qimm_layer_for_each_func_t)(struct qimm_shell *,
                                           struct weston_layer *,
                                           void *);
void
qimm_layer_for_each(struct qimm_shell *shell,
                    qimm_layer_for_each_func_t func, void *data);

/* --------- output --------- */
void
qimm_output_init(struct qimm_shell *shell);
void
qimm_output_release(struct qimm_shell *shell);
struct qimm_output *
qimm_output_get_default(struct qimm_shell *shell);
struct qimm_output *
qimm_output_find_by_name(struct qimm_shell *shell, const char *name);
void
qimm_output_project_insert(struct qimm_output *output,
                           struct wl_list *pos,
                           struct qimm_project *project);
void
qimm_output_project_remove(struct qimm_project *project);
/*
 * move projects to another output when its output has been destroyed
 */
int
qimm_output_project_move(struct qimm_output *from);
/*
 * restore projects to its output by name when a new output created
 */
void
qimm_output_project_restore(struct qimm_output *to);

/* --------- process --------- */
struct wl_client *
qimm_process_launch(struct weston_compositor *compositor, char *const argv[]);

/* --------- client --------- */
int
qimm_client_init(struct qimm_shell *shell);
void
qimm_client_release(struct qimm_shell *shell);
void
qimm_client_start(struct qimm_client_startup *startup);
void
qimm_client_project_start(struct qimm_project *project);

/* --------- project --------- */
struct qimm_project *
qimm_project_create(struct qimm_shell *shell,
                    const char *name,
                    const char *config_name);
struct qimm_project *
qimm_project_create_dashboard(struct qimm_shell *shell);
struct qimm_project *
qimm_project_create_assistant(struct qimm_shell *shell);
void
qimm_project_destroy(struct qimm_project *project);

int
qimm_project_load(struct qimm_shell *shell);
void
qimm_project_unload(struct qimm_shell *shell);

void
qimm_project_show(struct qimm_project *project);

/* --------- config --------- */
void *
qimm_config_project_load(const char *name, const char *config_name);
void
qimm_config_project_free(void *project_config);

/* --------- layout --------- */
int
qimm_layout_project_init(struct qimm_project *project);
void
qimm_layout_project_clear(struct qimm_project *project);
/*
 * find layout for wayland client sureface
 * NOTE: find in all output and all project in shell
 *       for project preload reseaon
 */
struct qimm_layout *
qimm_layout_find_by_client(struct qimm_shell *shell, struct wl_client *client);
int
qimm_layout_project_update(struct qimm_project *project);

/* --------- data --------- */
char *
qimm_data_path(void);
int
qimm_data_save_project(struct qimm_project *project);
void *
qimm_data_read_project(struct qimm_shell *shell, const char *name);
int
qimm_data_save_background(struct qimm_project *project,
                          struct qimm_data_background *background);

/* --------- shell --------- */
struct qimm_output *
qimm_shell_get_focus_output(struct qimm_shell *shell);
struct qimm_project *
qimm_shell_get_current_project(struct qimm_shell *shell);

#ifdef  __cplusplus
}
#endif

#endif // QIMM_H
