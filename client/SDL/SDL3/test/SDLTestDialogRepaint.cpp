/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * SDL client regression tests
 * SPDX-License-Identifier: Apache-2.0
 */

#include <SDL3/SDL.h>

extern "C" bool SDLCALL checkedRenderPresent(SDL_Renderer* renderer);
#define SDL_RenderPresent checkedRenderPresent
#include "../dialogs/sdl_widget_list.cpp"
#include "../dialogs/sdl_input_widget_pair_list.cpp"
#undef SDL_RenderPresent
