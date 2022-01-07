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
#include "clients/window.h"

struct client {
    struct display *display;
    struct window *window;
    struct widget *widget;

    char *project;
    char *app;
};

static void
redraw_handler(struct widget *widget, void *data) {
    struct rectangle allocation;
    widget_get_allocation(widget, &allocation);
    if (allocation.width == 0)
        return;

    struct client *client = data;
    cairo_t *cr = widget_cairo_create(client->widget);

    float rgb[3];
    random_rgb(rgb);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, rgb[0], rgb[1], rgb[2], 1.0);
    cairo_paint(cr);

    cairo_text_extents_t extents;
    cairo_set_font_size(cr, 14);
    cairo_text_extents(cr, client->app, &extents);
    if (allocation.x > 0)
        allocation.x += allocation.width - extents.width;
    else
        allocation.x += allocation.width / 2 - extents.width / 2;
    allocation.y += allocation.height / 2 - 1 + extents.height / 2;
    cairo_move_to(cr, allocation.x + 1, allocation.y + 1);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.85);
    cairo_show_text(cr, client->app);
    cairo_move_to(cr, allocation.x, allocation.y);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.85);
    cairo_show_text(cr, client->app);

    cairo_destroy(cr);
}

static int
run(struct client *client) {
    client->window = window_create(client->display);
    client->widget = window_add_widget(client->window, client);
    window_set_title(client->window, "qimm-client");

    widget_set_redraw_handler(client->widget, redraw_handler);
    // widget_set_button_handler(client.widget, button_handler);
    widget_set_default_cursor(client->widget, CURSOR_LEFT_PTR);
    // widget_set_touch_down_handler(client.widget, touch_down_handler);

    display_run(client->display);

    widget_destroy(client->widget);
    window_destroy(client->window);
    display_destroy(client->display);
    return 0;
}

static void
usage(int error_code) {
    FILE *out = error_code == EXIT_SUCCESS ? stdout : stderr;
    fprintf(out, "Usage: qimm-client [OPTIONS]\n"
                 "\n"
                 "This is qimm version " QIMM_VERSION "\n"
                 "A Situational Linux Desktop Based on Weston.\n"
                 "\n"
                 "Core options:\n"
                 "\n"
                 "  -p, --project\t\tWhich project this client belongs to\n"
                 "  -a, --app\t\tWhich application this client run as\n"
                 "  -h, --help\t\tThis help message\n\n");

    exit(error_code);
}

int
main(int argc, char **argv) {
// #ifdef DEBUG
#ifndef DEBUG
    printf("Qimm PID is %ld - "
               "waiting for debugger, send SIGCONT to continue...\n",
            (long)getpid());
    raise(SIGSTOP);
#endif

    struct client client = {0};
    const struct option long_options[] = {
            {"help",    no_argument,       NULL, 'h'},
            {"project", required_argument, NULL, 'p'},
            {"app",     required_argument, NULL, 'a'},
            {0, 0, 0,                            0}
    };
    while (1) {
        int i = 0;
        int c = getopt_long(argc, argv, "hp:a:", long_options, &i);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'h': // help
                usage(EXIT_SUCCESS);
                break;
            case 'p': // project
                client.project = optarg;
                break;
            case 'a': // app
                client.app = optarg;
                break;
            default:
                usage(EXIT_FAILURE);
        }
    }
    if (!client.project || !client.app) {
        fprintf(stderr, "The qimm-client must belongs to a project "
                        "and run as a application.\n\n");
        usage(EXIT_FAILURE);
    }

    client.display = display_create(&argc, argv);
    if (client.display == NULL) {
        fprintf(stderr, "failed to create display: %s\n",
                strerror(errno));
        return -1;
    }
    return run(&client);
}
