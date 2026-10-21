/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * SDL client regression tests
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdl_window.hpp"
#include <cstdio>
#include <cstdlib>
#include <memory>
static void require(bool ok, const char* message)
{
	if (!ok)
	{
		std::fprintf(stderr, "%s: %s\n", message, SDL_GetError());
		std::exit(2);
	}
}
int main()
{
	require(SDL_Init(SDL_INIT_VIDEO), "init");
	{
		auto window = SdlWindow::create(SDL_GetPrimaryDisplay(), "clipped RAIL repaint", 0, 64, 64);
		require(window.renderer() != nullptr, "renderer");
		std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> source(
		    SDL_CreateSurface(64, 64, SDL_PIXELFORMAT_BGRA32), SDL_DestroySurface);
		require(source != nullptr, "surface");
		require(SDL_FillSurfaceRect(source.get(), nullptr,
		                            SDL_MapSurfaceRGBA(source.get(), 255, 0, 0, 255)),
		        "fill source");
		require(window.fill(static_cast<Uint8>(0), 0, 0, 0),
		        "clear transparent like layered RAIL window");
		require(window.drawRects(source.get(), { 0, 0 }, { { 0, 0, 8, 8 } }),
		        "draw allowed visibility rect");
		std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> result(
		    SDL_RenderReadPixels(window.renderer(), nullptr), SDL_DestroySurface);
		require(result != nullptr, "read target");
		Uint8 r, g, b, a;
		require(SDL_ReadSurfacePixel(result.get(), 48, 48, &r, &g, &b, &a), "read excluded pixel");
		std::printf("Outside allowed visibility rect: RGBA=%u,%u,%u,%u (expected transparent)\n", r,
		            g, b, a);
		if (a != 0)
			return 1;
	}
	SDL_Quit();
	return 0;
}
