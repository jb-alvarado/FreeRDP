/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * SDL client regression tests
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dialogs/sdl_widget_list.hpp"
#include <cstdio>
#include <cstdlib>
static float density = 1.0f;
extern "C" float testPixelDensity(SDL_Window*)
{
	return density;
}
static void require(bool ok, const char* message)
{
	if (!ok)
	{
		std::fprintf(stderr, "FAIL: %s (%s)\n", message, SDL_GetError());
		std::exit(1);
	}
}
class Dialog : public SdlWidgetList
{
  public:
	using SdlWidgetList::update;
	SDL_Renderer* renderer()
	{
		return _renderer.get();
	}

  protected:
	bool updateInternal() override
	{
		return true;
	}
};
int main()
{
	require(SDL_Init(SDL_INIT_VIDEO), "SDL init");
	{
		Dialog dialog;
		require(dialog.reset("DPI regression", 100, 50), "dialog reset");
		for (float scale : { 1.0f, 2.0f, 1.25f, 1.0f })
		{
			density = scale;
			require(dialog.update(), "dialog repaint after density change");
			float x = 0, y = 0;
			require(SDL_GetRenderScale(dialog.renderer(), &x, &y), "read renderer scale");
			require(x == scale && y == scale, "renderer follows new monitor density");
		}
	}
	SDL_Quit();
	std::puts("PASS: dialog follows monitor pixel density 1 -> 2 -> 1.25 -> 1");
}
