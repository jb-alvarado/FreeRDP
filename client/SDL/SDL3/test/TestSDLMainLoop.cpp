/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * SDL client regression tests
 * SPDX-License-Identifier: Apache-2.0
 */

#define main freerdp_original_main
#include "../sdl_freerdp.cpp"
#undef main
#include <atomic>
#include <thread>
static std::atomic<int> sends{ 0 };
static void require(bool ok, const char* message)
{
	if (!ok)
	{
		std::fprintf(stderr, "FAIL: %s (%s)\n", message, SDL_GetError());
		std::exit(1);
	}
}
static UINT recordLayout(DispClientContext* channel, UINT32 count,
                         DISPLAY_CONTROL_MONITOR_LAYOUT* layout)
{
	require(count == 1 && layout[0].Width == 1800 && layout[0].Height == 1100,
	        "actual enlarged window geometry");
	sends++;
	require(SetEvent(freerdp_abort_event(static_cast<rdpContext*>(channel->handle))),
	        "stop main loop");
	return CHANNEL_RC_OK;
}
int main()
{
	require(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), "SDL init");
	auto instance = freerdp_new();
	require(instance != nullptr, "instance");
	instance->ContextSize = sizeof(sdl_rdp_context);
	require(freerdp_context_new(instance), "context");
	auto context = instance->context;
	auto settings = context->settings;
	require(freerdp_settings_set_string(settings, FreeRDP_WindowTitle, "Main loop test"), "title");
	require(freerdp_settings_set_bool(settings, FreeRDP_DynamicResolutionUpdate, TRUE),
	        "dynamic resize");
	require(freerdp_settings_set_uint32(settings, FreeRDP_DesktopWidth, 1800), "initial width");
	require(freerdp_settings_set_uint32(settings, FreeRDP_DesktopHeight, 1100), "initial height");
	rdpMonitor monitor{};
	monitor.width = 1800;
	monitor.height = 1100;
	monitor.is_primary = TRUE;
	require(freerdp_settings_set_monitor_def_array_sorted(settings, &monitor, 1), "monitor");
	{
		SdlContext sdl(context);
		reinterpret_cast<sdl_rdp_context*>(context)->sdl = &sdl;
		sdl.setMonitorIds({ SDL_GetPrimaryDisplay() });
		require(sdl.createWindows(), "window");
		// New desktop negotiates 1024x768 while retaining the enlarged window.
		monitor.width = 1024;
		monitor.height = 768;
		require(freerdp_settings_set_monitor_def_array_sorted(settings, &monitor, 1),
		        "stale monitor");
		require(freerdp_settings_set_uint32(settings, FreeRDP_DesktopWidth, 1024), "desktop width");
		require(freerdp_settings_set_uint32(settings, FreeRDP_DesktopHeight, 768),
		        "desktop height");
		auto& disp = sdl.getDisplayChannelContext();
		DispClientContext channel{};
		channel.handle = context;
		channel.SendMonitorLayout = recordLayout;
		require(disp.init(&channel), "channel");
		SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
		SDL_Event reply{};
		reply.type = SDL_EVENT_USER_CERT_RESULT;
		require(SDL_PushEvent(&reply), "worker result");
		require(channel.DisplayControlCaps(&channel, 1, 8192, 8192) == CHANNEL_RC_OK, "caps");
		std::thread watchdog(
		    [context]
		    {
			    SDL_Delay(1500);
			    SetEvent(freerdp_abort_event(context));
			    SDL_Event quit{};
			    quit.type = SDL_EVENT_QUIT;
			    SDL_PushEvent(&quit);
		    });
		std::ignore = sdl_run(&sdl);
		watchdog.join();
		require(sends == 1, "main loop refreshes window geometry before the delayed layout is sent");
		require(SDL_PeepEvents(&reply, 1, SDL_GETEVENT, SDL_EVENT_USER_CERT_RESULT,
		                       SDL_EVENT_USER_CERT_RESULT) == 1,
		        "dialog result stays queued for worker");
		require(disp.uninit(&channel), "uninit");
		sdl.cleanup();
	}
	instance->PostDisconnect = nullptr;
	instance->PostFinalDisconnect = nullptr;
	freerdp_context_free(instance);
	freerdp_free(instance);
	SDL_Quit();
	std::puts(
	    "PASS: real SDL main loop processes automatic layout event and preserves worker replies");
}
