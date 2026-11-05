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

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include "toolbar.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static void frontend_event_callback(obs_frontend_event event, void *private_data)
{
	UNUSED_PARAMETER(private_data);

	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		/* A barra deve ser criada depois que a UI principal do OBS
		 * terminou de carregar (é um evento da thread de UI). */
		quickres_create_dock();
		break;
	default:
		break;
	}
}

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	obs_frontend_add_event_callback(frontend_event_callback, nullptr);
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(frontend_event_callback, nullptr);
	obs_log(LOG_INFO, "plugin unloaded");
}
