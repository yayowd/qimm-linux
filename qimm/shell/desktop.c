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

static int
qimm_surface_get_label(struct weston_surface *surface, char *buf, size_t len) {
    const char *t, *c;
    struct weston_desktop_surface *desktop_surface =
            weston_surface_get_desktop_surface(surface);

    t = weston_desktop_surface_get_title(desktop_surface);
    c = weston_desktop_surface_get_app_id(desktop_surface);

    return snprintf(buf, len, "%s window%s%s%s%s%s",
            "top-level",
            t ? " '" : "", t ?: "", t ? "'" : "",
            c ? " of " : "", c ?: "");
}

/*
 * Returns the bounding box of a surface and all its sub-surfaces,
 * in surface-local coordinates. */
static void
surface_subsurfaces_boundingbox(struct weston_surface *surface, int32_t *x,
        int32_t *y, int32_t *w, int32_t *h) {
    pixman_region32_t region;
    pixman_box32_t *box;
    struct weston_subsurface *subsurface;

    pixman_region32_init_rect(&region, 0, 0,
            surface->width,
            surface->height);

    wl_list_for_each(subsurface, &surface->subsurface_list, parent_link) {
        pixman_region32_union_rect(&region, &region,
                subsurface->position.x,
                subsurface->position.y,
                subsurface->surface->width,
                subsurface->surface->height);
    }

    box = pixman_region32_extents(&region);
    if (x)
        *x = box->x1;
    if (y)
        *y = box->y1;
    if (w)
        *w = box->x2 - box->x1;
    if (h)
        *h = box->y2 - box->y1;

    pixman_region32_fini(&region);
}

static void
center_on_output(struct weston_view *view, struct weston_output *output) {
    int32_t surf_x, surf_y, width, height;
    float x, y;

    if (!output) {
        weston_view_set_position(view, 0, 0);
        return;
    }

    surface_subsurfaces_boundingbox(view->surface, &surf_x, &surf_y, &width, &height);

    x = output->x + (output->width - width) / 2 - surf_x / 2;
    y = output->y + (output->height - height) / 2 - surf_y / 2;

    weston_view_set_position(view, x, y);
}

static void
qimm_view_set_position(struct weston_view *view,
        struct qimm_layout *layout, struct qimm_shell *shell) {
    if (layout)
        weston_view_set_position(view, layout->x, layout->y);
    else { /* freedom views */
        if (shell->output_focus)
            center_on_output(view, shell->output_focus->output);
        else
            weston_view_set_position(view, 0, 0);
    }
}

static void
qimm_surface_update_layer(struct qimm_surface *qimm_surface,
        struct qimm_shell *shell) {
    struct qimm_project *project = qimm_shell_get_current_project(shell);
    if (!project) {
        return;
    }
    /*
     * the first layout is for background
     */
    struct weston_layer_entry *new_layer_link;
    if (project->layouts.next == &qimm_surface->layout->link)
        new_layer_link = &project->output->shell->background_layer.view_list;
    else
        new_layer_link = &project->layer.view_list;
    if (new_layer_link == &qimm_surface->view->layer_link) {
        return;
    }

    weston_view_geometry_dirty(qimm_surface->view);
    weston_layer_entry_remove(&qimm_surface->view->layer_link);
    weston_layer_entry_insert(new_layer_link, &qimm_surface->view->layer_link);
    weston_view_geometry_dirty(qimm_surface->view);

    struct weston_surface *surface =
            weston_desktop_surface_get_surface(qimm_surface->desktop_surface);
    weston_surface_damage(surface);

    weston_desktop_surface_propagate_layer(qimm_surface->desktop_surface);
}

static struct qimm_surface *
get_qimm_surface(struct weston_surface *surface) {
    if (weston_surface_is_desktop_surface(surface)) {
        struct weston_desktop_surface *desktop_surface =
                weston_surface_get_desktop_surface(surface);
        return weston_desktop_surface_get_user_data(desktop_surface);
    }
    return NULL;
}

static struct qimm_surface *
get_last_child(struct qimm_surface *qimm_surface) {
    struct qimm_surface *qimm_surface_child;
    wl_list_for_each_reverse(qimm_surface_child, &qimm_surface->children_list, children_link) {
        if (weston_view_is_mapped(qimm_surface_child->view))
            return qimm_surface_child;
    }
    return NULL;
}

static void
activate(struct qimm_shell *shell, struct weston_view *view,
        struct weston_seat *seat, uint32_t flags) {
    struct weston_surface *es = view->surface;
    struct weston_surface *main_surface;
    struct qimm_surface *qimm_surface, *qimm_surface_child;

    main_surface = weston_surface_get_main_surface(es);
    qimm_surface = get_qimm_surface(main_surface);
    assert(qimm_surface);

    qimm_surface_child = get_last_child(qimm_surface);
    if (qimm_surface_child) {
        /* Activate last xdg child instead of parent. */
        activate(shell, qimm_surface_child->view, seat, flags);
        return;
    }

    weston_view_activate_input(view, seat, flags);
}

static struct weston_view *
get_default_view(struct weston_surface *surface) {
    if (!surface || wl_list_empty(&surface->views))
        return NULL;

    struct qimm_surface *qimm_surface = get_qimm_surface(surface);
    if (qimm_surface)
        return qimm_surface->view;

    struct weston_view *view;
    wl_list_for_each(view, &surface->views, surface_link) {
        if (weston_view_is_mapped(view))
            return view;
    }

    return container_of(surface->views.next, struct weston_view, surface_link);
}

static void
map(struct qimm_shell *shell, struct qimm_surface *qimm_surface,
        int32_t sx, int32_t sy) {
    qimm_view_set_position(qimm_surface->view, qimm_surface->layout, shell);

    qimm_surface_update_layer(qimm_surface, shell);

    weston_view_update_transform(qimm_surface->view);
    qimm_surface->view->is_mapped = true;

    if (!shell->locked) {
        struct weston_seat *seat;
        wl_list_for_each(seat, &shell->compositor->seat_list, link) {
            activate(shell, qimm_surface->view, seat,
                    WESTON_ACTIVATE_FLAG_CONFIGURE);
        }
    }
}

static void
desktop_surface_added(struct weston_desktop_surface *desktop_surface,
        void *shell) {
    struct weston_desktop_client *client =
            weston_desktop_surface_get_client(desktop_surface);
    struct wl_client *wl_client =
            weston_desktop_client_get_client(client);
    struct weston_surface *surface =
            weston_desktop_surface_get_surface(desktop_surface);

    struct weston_view *view
            = weston_desktop_surface_create_view(desktop_surface);
    if (!view)
        return;

    struct qimm_surface *qimm_surface = zalloc(sizeof *qimm_surface);
    if (!qimm_surface) {
        if (wl_client)
            wl_client_post_no_memory(wl_client);
        else
            qimm_log("no memory to allocate qimm surface");
        return;
    }

    weston_surface_set_label_func(surface, qimm_surface_get_label);

    qimm_surface->desktop_surface = desktop_surface;
    qimm_surface->view = view;

    wl_signal_init(&qimm_surface->destroy_signal);

    /*
     * initialize list as well as link. The latter allows to use
     * wl_list_remove() even when this surface is not in another list.
     */
    wl_list_init(&qimm_surface->children_list);
    wl_list_init(&qimm_surface->children_link);

    qimm_surface->layout = qimm_layout_find_by_client(shell, wl_client);
    if (qimm_surface->layout)
        weston_desktop_surface_set_size(desktop_surface,
                qimm_surface->layout->w, qimm_surface->layout->h);

    weston_desktop_surface_set_user_data(desktop_surface, qimm_surface);
    weston_desktop_surface_set_activated(desktop_surface, false);
}

static void
desktop_surface_removed(struct weston_desktop_surface *desktop_surface,
        void *shell) {
    struct qimm_surface *qimm_surface =
            weston_desktop_surface_get_user_data(desktop_surface);
    struct weston_surface *surface =
            weston_desktop_surface_get_surface(desktop_surface);

    if (!qimm_surface)
        return;

    struct qimm_surface *qimm_surface_child, *tmp;
    wl_list_for_each_safe(qimm_surface_child, tmp,
            &qimm_surface->children_list, children_link) {
        wl_list_remove(&qimm_surface_child->children_link);
        wl_list_init(&qimm_surface_child->children_link);
    }
    wl_list_remove(&qimm_surface->children_link);

    wl_signal_emit(&qimm_surface->destroy_signal, qimm_surface);

    weston_surface_set_label_func(surface, NULL);
    weston_desktop_surface_set_user_data(qimm_surface->desktop_surface, NULL);

    weston_desktop_surface_unlink_view(qimm_surface->view);
    weston_view_destroy(qimm_surface->view);

    free(qimm_surface);
}

static void
desktop_surface_committed(struct weston_desktop_surface *desktop_surface,
        int32_t sx, int32_t sy, void *data) {
    struct qimm_surface *qimm_surface =
            weston_desktop_surface_get_user_data(desktop_surface);
    struct weston_surface *surface =
            weston_desktop_surface_get_surface(desktop_surface);
    struct weston_view *view = qimm_surface->view;
    struct qimm_shell *shell = data;

    if (surface->width == 0)
        return;

    if (!weston_surface_is_mapped(surface)) {
        map(shell, qimm_surface, sx, sy);
        surface->is_mapped = true;
        return;
    }

    qimm_view_set_position(qimm_surface->view, qimm_surface->layout, shell);

    /* XXX: would a fullscreen surface need the same handling? */
    if (surface->output) {
        wl_list_for_each(view, &surface->views, surface_link) {
            weston_view_update_transform(view);
        }
    }
}

static void
desktop_surface_move(struct weston_desktop_surface *desktop_surface,
        struct weston_seat *seat, uint32_t serial, void *shell) {
//    struct weston_pointer *pointer = weston_seat_get_pointer(seat);
//    struct weston_touch *touch = weston_seat_get_touch(seat);
//    struct weston_surface *surface =
//            weston_desktop_surface_get_surface(desktop_surface);
//    struct wl_resource *resource = surface->resource;

//    struct shell_surface *shsurf =
//            weston_desktop_surface_get_user_data(desktop_surface);
//    struct weston_surface *focus;
//
//    if (pointer &&
//        pointer->focus &&
//        pointer->button_count > 0 &&
//        pointer->grab_serial == serial) {
//        focus = weston_surface_get_main_surface(pointer->focus->surface);
//        if ((focus == surface) &&
//            (surface_move(shsurf, pointer, true) < 0))
//            wl_resource_post_no_memory(resource);
//    } else if (touch &&
//               touch->focus &&
//               touch->grab_serial == serial) {
//        focus = weston_surface_get_main_surface(touch->focus->surface);
//        if ((focus == surface) &&
//            (surface_touch_move(shsurf, touch) < 0))
//            wl_resource_post_no_memory(resource);
//    }
}

static void
desktop_surface_resize(struct weston_desktop_surface *desktop_surface,
        struct weston_seat *seat, uint32_t serial,
        enum weston_desktop_surface_edge edges, void *shell) {
//    struct weston_pointer *pointer = weston_seat_get_pointer(seat);

//    struct shell_surface *shsurf =
//            weston_desktop_surface_get_user_data(desktop_surface);
//    struct weston_surface *surface =
//            weston_desktop_surface_get_surface(shsurf->desktop_surface);
//    struct wl_resource *resource = surface->resource;
//    struct weston_surface *focus;
//
//    if (!pointer ||
//        pointer->button_count == 0 ||
//        pointer->grab_serial != serial ||
//        pointer->focus == NULL)
//        return;
//
//    focus = weston_surface_get_main_surface(pointer->focus->surface);
//    if (focus != surface)
//        return;
//
//    if (surface_resize(shsurf, pointer, edges) < 0)
//        wl_resource_post_no_memory(resource);
}

static void
desktop_surface_set_parent(struct weston_desktop_surface *desktop_surface,
        struct weston_desktop_surface *parent,
        void *shell) {
//    struct shell_surface *shsurf_parent;
//    struct shell_surface *shsurf =
//            weston_desktop_surface_get_user_data(desktop_surface);
//
//    /* unlink any potential child */
//    wl_list_remove(&shsurf->children_link);
//
//    if (parent) {
//        shsurf_parent = weston_desktop_surface_get_user_data(parent);
//        wl_list_insert(shsurf_parent->children_list.prev,
//                &shsurf->children_link);
//    } else {
//        wl_list_init(&shsurf->children_link);
//    }
}

static void
desktop_surface_fullscreen_requested(struct weston_desktop_surface *desktop_surface,
        bool fullscreen,
        struct weston_output *output, void *shell) {
//    struct shell_surface *shsurf =
//            weston_desktop_surface_get_user_data(desktop_surface);
//
//    set_fullscreen(shsurf, fullscreen, output);
}


static void
desktop_surface_maximized_requested(struct weston_desktop_surface *desktop_surface,
        bool maximized, void *shell) {
//    struct shell_surface *shsurf =
//            weston_desktop_surface_get_user_data(desktop_surface);
//
//    set_maximized(shsurf, maximized);
}

static void
desktop_surface_minimized_requested(struct weston_desktop_surface *desktop_surface,
        void *shell) {
//    struct weston_surface *surface =
//            weston_desktop_surface_get_surface(desktop_surface);
//
//    /* apply compositor's own minimization logic (hide) */
//    set_minimized(surface);
}

static void
desktop_surface_ping_timeout(struct weston_desktop_client *desktop_client,
        void *shell_) {
//    struct desktop_shell *shell = shell_;
//    struct shell_surface *shsurf;
//    struct weston_seat *seat;
//    bool unresponsive = true;
//
//    weston_desktop_client_for_each_surface(desktop_client,
//            desktop_surface_set_unresponsive,
//            &unresponsive);
//
//
//    wl_list_for_each(seat, &shell->compositor->seat_list, link) {
//        struct weston_pointer *pointer = weston_seat_get_pointer(seat);
//        struct weston_desktop_client *grab_client;
//
//        if (!pointer || !pointer->focus)
//            continue;
//
//        shsurf = get_shell_surface(pointer->focus->surface);
//        if (!shsurf)
//            continue;
//
//        grab_client =
//                weston_desktop_surface_get_client(shsurf->desktop_surface);
//        if (grab_client == desktop_client)
//            set_busy_cursor(shsurf, pointer);
//    }
}

static void
desktop_surface_pong(struct weston_desktop_client *desktop_client,
        void *shell_) {
//    struct desktop_shell *shell = shell_;
//    bool unresponsive = false;
//
//    weston_desktop_client_for_each_surface(desktop_client,
//            desktop_surface_set_unresponsive,
//            &unresponsive);
//    end_busy_cursor(shell->compositor, desktop_client);
}

static void
desktop_surface_set_xwayland_position(struct weston_desktop_surface *surface,
        int32_t x, int32_t y, void *shell_) {
//    struct shell_surface *shsurf =
//            weston_desktop_surface_get_user_data(surface);
//
//    shsurf->xwayland.x = x;
//    shsurf->xwayland.y = y;
//    shsurf->xwayland.is_set = true;
}

const struct weston_desktop_api qimm_shell_desktop_api = {
        .struct_size = sizeof(struct weston_desktop_api),
        .surface_added = desktop_surface_added,
        .surface_removed = desktop_surface_removed,
        .committed = desktop_surface_committed,
        .move = desktop_surface_move,
        .resize = desktop_surface_resize,
        .set_parent = desktop_surface_set_parent,
        .fullscreen_requested = desktop_surface_fullscreen_requested,
        .maximized_requested = desktop_surface_maximized_requested,
        .minimized_requested = desktop_surface_minimized_requested,
        .ping_timeout = desktop_surface_ping_timeout,
        .pong = desktop_surface_pong,
        .set_xwayland_position = desktop_surface_set_xwayland_position,
};
