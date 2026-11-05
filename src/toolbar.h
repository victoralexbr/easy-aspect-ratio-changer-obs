/*
Toolbar de Resolução
Copyright (C) 2026 victoralexbr <victoralexbr@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

/* Cria a dock "Toolbar de Resolução" no OBS. Deve ser chamado a partir
 * da thread de UI (ex.: no evento OBS_FRONTEND_EVENT_FINISHED_LOADING). */
void quickres_create_dock();
