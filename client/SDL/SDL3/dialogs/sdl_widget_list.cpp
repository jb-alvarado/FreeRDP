#include "sdl_widget_list.hpp"
#include "sdl_blend_mode_guard.hpp"

SdlWidgetList::~SdlWidgetList() = default;

bool SdlWidgetList::reset(const std::string& title, size_t width, size_t height)
{
	auto w = WINPR_ASSERTING_INT_CAST(int, width);
	auto h = WINPR_ASSERTING_INT_CAST(int, height);
	SDL_Renderer* renderer = nullptr;
	SDL_Window* window = nullptr;
	auto rc = SDL_CreateWindowAndRenderer(title.c_str(), w, h,
	                                      SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_ALWAYS_ON_TOP |
	                                          SDL_WINDOW_UTILITY,
	                                      &window, &renderer);
	_renderer = std::shared_ptr<SDL_Renderer>(renderer, SDL_DestroyRenderer);
	_window = std::shared_ptr<SDL_Window>(window, SDL_DestroyWindow);
	if (!rc)
		widget_log_error(rc, "SDL_CreateWindowAndRenderer");
	else
		rc = updateScale();
	return rc;
}

bool SdlWidgetList::updateScale()
{
	/* Re-read the density when drawing: moving between monitors can change it after creation. */
	const float density = SDL_GetWindowPixelDensity(_window.get());
	if (widget_log_error(density > 0.0f, "SDL_GetWindowPixelDensity"))
		return false;
	float scaleX = 0.0f;
	float scaleY = 0.0f;
	if (widget_log_error(SDL_GetRenderScale(_renderer.get(), &scaleX, &scaleY),
	                     "SDL_GetRenderScale"))
		return false;
	if ((scaleX == density) && (scaleY == density))
		return true;
	const auto rc = SDL_SetRenderScale(_renderer.get(), density, density);
	return !widget_log_error(rc, "SDL_SetRenderScale");
}

bool SdlWidgetList::visible() const
{
	if (!_window || !_renderer)
		return false;

	auto flags = SDL_GetWindowFlags(_window.get());
	return (flags & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED)) == 0;
}

bool SdlWidgetList::clearWindow()
{
	if (!_renderer)
		return false;

	SdlBlendModeGuard guard(_renderer, SDL_BLENDMODE_NONE);
	const auto drc = SDL_SetRenderDrawColor(_renderer.get(), _backgroundcolor.r, _backgroundcolor.g,
	                                        _backgroundcolor.b, _backgroundcolor.a);
	if (widget_log_error(drc, "SDL_SetRenderDrawColor"))
		return false;

	const auto rcls = SDL_RenderClear(_renderer.get());
	return !widget_log_error(rcls, "SDL_RenderClear");
}

bool SdlWidgetList::update()
{
	if (!visible())
		return true;

	if (!updateScale())
		return false;
	if (!clearWindow())
		return false;
	if (!updateInternal())
		return false;
	if (!_buttons.update())
		return false;
	auto rc = SDL_RenderPresent(_renderer.get());
	if (!rc)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] SDL_RenderPresent failed with %s", __func__,
		            SDL_GetError());
	}
	return rc;
}
