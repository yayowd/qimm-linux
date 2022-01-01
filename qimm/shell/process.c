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

/** NOTE:
 *
 * copy from compositor/main.c: child_client_exec
 * support command line agrs
 */
static void
qimm_process_exec(int sockfd, char *const argv[]) {
    char *cmd = get_command_line(argv);
    if (!cmd)
        goto final;

    /* do not give our signal mask to the new process */
    sigset_t allsigs;
    sigfillset(&allsigs);
    sigprocmask(SIG_UNBLOCK, &allsigs, NULL);

    /* Launch clients as the user. Do not launch clients with wrong euid. */
    if (seteuid(getuid()) == -1) {
        qimm_log("process exec error: [%s] failed seteuid", cmd);
        goto final;
    }

    /* SOCK_CLOEXEC closes both ends, so we dup the fd to get a
     * non-CLOEXEC fd to pass through exec. */
    int clientfd = dup(sockfd);
    if (clientfd == -1) {
        qimm_log("process exec error: [%s] dup failed: %s",
                cmd, strerror(errno));
        goto final;
    }

    char s[32];
    snprintf(s, sizeof s, "%d", clientfd);
    setenv("WAYLAND_SOCKET", s, 1);

    execv(argv[0], argv);
    /* exec- returned on error only */
    qimm_log("process exec error: [%s] executing failed: %s",
            cmd, strerror(errno));

final:
    free(cmd);
}

/** NOTE:
 *
 * copy from compositor/main.c: weston_client_launch
 * support command line agrs and use vfork to wait child process to exec or exit
 */
struct wl_client *
qimm_process_launch(struct weston_compositor *compositor, char *const argv[]) {
    char *cmd = get_command_line(argv);
    if (!cmd)
        return NULL;
    qimm_log("process launching: [%s]", cmd);

    int sv[2] = {0, 0};
    if (os_socketpair_cloexec(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        qimm_log("process launch error: [%s] socketpair failed: %s",
                cmd, strerror(errno));
        free(cmd);
        return NULL;
    }

    struct wl_client *client = NULL;

    /*
     * wait child process to exec or exit
     * then check child process status in parent
     */
    pid_t pid = vfork();
    if (pid == -1) {
        qimm_log("process launch error: [%s] fork failed: %s",
                cmd, strerror(errno));
        goto final;
    }

    if (pid == 0) {
        qimm_process_exec(sv[1], argv);
        /* exit when error in child process */
        _exit(-1);
    }

    int status;
    pid_t wait_pid = waitpid(pid, &status, WNOHANG);
    if (wait_pid == -1) {
        qimm_log("process launch error: cannot get status for child process");
        goto final;
    } else if (wait_pid > 0) /* child process has been disappeared */
        goto final;

    client = wl_client_create(compositor->wl_display, sv[0]);
    if (!client)
        qimm_log("process launch error: [%s] wl_client_create failed", cmd);

final:
    if (!client)
        close(sv[0]);
    close(sv[1]);
    free(cmd);
    return client;
}
