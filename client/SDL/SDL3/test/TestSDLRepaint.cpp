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
		std::exit(1);
	}
}
static void pixel(SdlWindow& window, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
	std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> image(
	    SDL_RenderReadPixels(window.renderer(), nullptr), SDL_DestroySurface);
	require(image != nullptr, "read target");
	Uint8 actualR, actualG, actualB, actualA;
	require(SDL_ReadSurfacePixel(image.get(), x, y, &actualR, &actualG, &actualB, &actualA),
	        "read pixel");
	if (actualR != r || actualG != g || actualB != b || actualA != 255)
	{
		std::fprintf(stderr, "pixel %d,%d: got %u,%u,%u,%u expected %u,%u,%u,255\n", x, y, actualR,
		             actualG, actualB, actualA, r, g, b);
		std::exit(1);
	}
}
int main()
{
	require(SDL_Init(SDL_INIT_VIDEO), "init");
	{
		auto window = SdlWindow::create(SDL_GetPrimaryDisplay(), "repaint regression", 0, 64, 64);
		require(window.renderer() != nullptr, "renderer");
		std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(
		    SDL_CreateSurface(64, 64, SDL_PIXELFORMAT_BGRA32), SDL_DestroySurface);
		require(surface != nullptr, "surface");
		require(SDL_FillSurfaceRect(surface.get(), nullptr,
		                            SDL_MapSurfaceRGB(surface.get(), 255, 0, 0)),
		        "fill red");
		require(window.drawRects(surface.get(), { 0, 0 }, { { 0, 0, 4, 4 } }, true),
		        "initial dirty rect");
		pixel(window, 48, 48, 255, 0, 0);
		require(SDL_SetRenderTarget(window.renderer(), nullptr), "reset target");
		surface.reset(SDL_CreateSurface(16, 16, SDL_PIXELFORMAT_BGRA32));
		require(surface != nullptr, "smaller surface");
		require(SDL_FillSurfaceRect(surface.get(), nullptr,
		                            SDL_MapSurfaceRGB(surface.get(), 0, 0, 255)),
		        "fill blue");
		require(window.drawRects(surface.get(), { 0, 0 }, { { 0, 0, 4, 4 } }, true),
		        "desktop shrink");
		pixel(window, 12, 12, 0, 0, 255);
		pixel(window, 48, 48, 0, 0, 0);
		require(SDL_SetRenderTarget(window.renderer(), nullptr), "reset target");
		require(window.resize({ 96, 96 }), "grow window");
		require(SDL_SyncWindow(window.window()), "sync window");
		require(window.drawRects(surface.get(), { 0, 0 }, { { 0, 0, 4, 4 } }, true), "window grow");
		pixel(window, 12, 12, 0, 0, 255);
		pixel(window, 80, 80, 0, 0, 0);
		require(SDL_SetRenderTarget(window.renderer(), nullptr), "reset target");
		surface.reset(SDL_CreateSurface(24, 24, SDL_PIXELFORMAT_BGRA32));
		require(surface != nullptr, "scaled surface");
		require(SDL_FillSurfaceRect(surface.get(), nullptr,
		                            SDL_MapSurfaceRGB(surface.get(), 0, 255, 0)),
		        "fill green");
		require(window.drawScaledRects(surface.get(), { 2.0f, 2.0f }, { { 0, 0, 4, 4 } }, true),
		        "scaled desktop resize");
		pixel(window, 40, 40, 0, 255, 0);
		pixel(window, 80, 80, 0, 0, 0);
		require(SDL_SetRenderTarget(window.renderer(), nullptr), "reset target");
	}
	SDL_Quit();
	std::puts("PASS: initial partial update, desktop shrink, window growth, scaled desktop resize");
}
