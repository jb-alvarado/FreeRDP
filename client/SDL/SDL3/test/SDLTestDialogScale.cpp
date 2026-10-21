/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * SDL client regression tests
 * SPDX-License-Identifier: Apache-2.0
 */

#include <SDL3/SDL.h>

extern "C" float SDLCALL testPixelDensity(SDL_Window* window);
#define SDL_GetWindowPixelDensity testPixelDensity
#include "../dialogs/sdl_widget_list.cpp"
#undef SDL_GetWindowPixelDensity
