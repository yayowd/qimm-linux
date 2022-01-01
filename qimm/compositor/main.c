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

static void
usage(int error_code) {
    FILE *out = error_code == EXIT_SUCCESS ? stdout : stderr;
    fprintf(out,
            "Usage: qimm [OPTIONS]\n"
            "\n"
            "This is qimm version " QIMM_VERSION "\n"
            "A Situational Linux Desktop Based on Weston.\n"
            "\n"
            "Core options:\n"
            "\n"
            "  -v, --version\t\tPrint qimm version\n"
            "  -h, --help\t\tThis help message\n\n");

    exit(error_code);
}

static void
version(void) {
    fprintf(stdout,
            "%s\t%s\n"
            "Bug reports to:\t%s\n"
            "Build: %s\n"
            "\n"
            "Weston  version: %s\n"
            "Wayland version: %s\n",
            QIMM_PACKAGE_STRING, QIMM_PACKAGE_URL, QIMM_PACKAGE_BUGREPORT,
            QIMM_BUILD_ID,
            WESTON_VERSION,
            WAYLAND_VERSION);

    exit(EXIT_SUCCESS);
}

int
main(int argc, char **argv) {
    char *args[] = {argv[0],
                   // "--config=/home/wayland/weston-2.ini",
                    "--no-config",
                   // "--wait-for-debugger",
                    "--shell=qimm-shell.so"};

    const struct option long_options[] = {
            {"help",    no_argument, NULL, 'h'},
            {"version", no_argument, NULL, 'v'},
            {0, 0, 0,                      0}
    };
    while (1) {
        int i = 0;
        int c = getopt_long(argc, argv, "hv", long_options, &i);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'h': // help
                usage(EXIT_SUCCESS);
                break;
            case 'v': // version
                version();
                break;
            default:
                usage(EXIT_FAILURE);
        }
    }

#ifdef DEBUG
    if (setenv("WESTON_MODULE_MAP", WESTON_MODULE_MAP, 0) < 0) {
        fprintf(stderr, "Error: environment setup failed.\n");
        return EXIT_FAILURE;
    }
#endif

    return wet_main(ARRAY_LENGTH(args), args, NULL);
}
