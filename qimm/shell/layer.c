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

void
qimm_layer_init(struct qimm_shell *shell) {
    weston_layer_init(&shell->background_layer, shell->compositor);
    weston_layer_set_position(&shell->background_layer,
                              WESTON_LAYER_POSITION_BACKGROUND);
}

void
qimm_layer_release(struct qimm_shell *shell) {
    weston_layer_fini(&shell->background_layer);
}

void
qimm_layer_for_each(struct qimm_shell *shell,
        qimm_layer_for_each_func_t func, void *data) {
    func(shell, &shell->background_layer, data);

    struct qimm_output *output;
    wl_list_for_each(output, &shell->outputs, link) {
        if (output->project_cur)
            func(shell, &output->project_cur->layer, data);
    }
}
