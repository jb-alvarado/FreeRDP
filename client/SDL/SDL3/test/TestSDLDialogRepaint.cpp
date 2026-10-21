/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * SDL client regression tests
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdio>
#include <SDL3_ttf/SDL_ttf.h>
#include "dialogs/sdl_input_widget_pair_list.hpp"

static int frames = 0;

extern "C" bool SDLCALL checkedRenderPresent(SDL_Renderer* renderer)
{
	frames++;
	return SDL_RenderPresent(renderer);
}

int main()
{
	if (!SDL_Init(SDL_INIT_VIDEO))
		return 1;
	if (!TTF_Init())
	{
		SDL_Quit();
		return 1;
	}
	int result = 1;
	{
		SdlInputWidgetPairList dialog("Dialog repaint", { "Username", "Password" },
		                              { "user", "password" },
		                              { 0, SdlInputWidgetPair::SDL_INPUT_MASK });
		SDL_Event event{};
		event.type = SDL_EVENT_KEY_UP;
		event.key.key = SDLK_ESCAPE;
		if (SDL_PushEvent(&event))
		{
			std::vector<std::string> values;
			const int rc = dialog.run(values);
			std::fprintf(stderr, "dialog result=%d, presented frames=%d\n", rc, frames);
			result = (rc == -2 && frames == 1) ? 0 : 1;
		}
	}
	TTF_Quit();
	SDL_Quit();
	return result;
}
