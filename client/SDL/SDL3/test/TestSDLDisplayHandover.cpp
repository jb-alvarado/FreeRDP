/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * SDL client regression tests
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdl_context.hpp"
#include "sdl_utils.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
static std::atomic<int> sends{ 0 };
static UINT32 expectedWidth = 1600, expectedHeight = 1000;
static void require(bool ok, const char* message)
{
	if (!ok)
	{
		std::fprintf(stderr, "FAIL: %s (%s)\n", message, SDL_GetError());
		std::exit(1);
	}
}
static UINT sendLayout(DispClientContext*, UINT32 count, DISPLAY_CONTROL_MONITOR_LAYOUT* monitors)
{
	require(count == 1, "single monitor");
	if (monitors[0].Width != expectedWidth || monitors[0].Height != expectedHeight)
	{
		std::fprintf(stderr, "Requested %ux%u, expected actual window %ux%u\n", monitors[0].Width,
		             monitors[0].Height, expectedWidth, expectedHeight);
		std::exit(1);
	}
	sends++;
	return CHANNEL_RC_OK;
}
static void processLayoutEvents(sdlDispContext& disp)
{
	SDL_Event ev{};
	while (SDL_PollEvent(&ev))
		if (ev.type == SDL_EVENT_USER_UPDATE_MONITORS)
			require(disp.refreshLayout(), "main-thread layout refresh");
}
static void serverSize(rdpSettings* settings, UINT32 width, UINT32 height)
{
	require(freerdp_settings_set_uint32(settings, FreeRDP_DesktopWidth, width), "server width");
	require(freerdp_settings_set_uint32(settings, FreeRDP_DesktopHeight, height), "server height");
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
	require(freerdp_settings_set_string(settings, FreeRDP_WindowTitle, "Display handover test"),
	        "window title");
	require(freerdp_settings_set_bool(settings, FreeRDP_DynamicResolutionUpdate, TRUE),
	        "dynamic resolution");
	serverSize(settings, 1600, 1000);
	rdpMonitor monitor{};
	monitor.width = 1600;
	monitor.height = 1000;
	monitor.is_primary = TRUE;
	require(freerdp_settings_set_monitor_def_array_sorted(settings, &monitor, 1), "monitor");
	{
		SdlContext sdl(context);
		reinterpret_cast<sdl_rdp_context*>(context)->sdl = &sdl;
		sdl.setMonitorIds({ SDL_GetPrimaryDisplay() });
		require(sdl.createWindows(), "create desktop window");
		auto& disp = sdl.getDisplayChannelContext();
		DispClientContext channel{};
		channel.SendMonitorLayout = sendLayout;
		require(disp.init(&channel), "channel init");
		require(channel.DisplayControlCaps(&channel, 1, 4096, 4096) == CHANNEL_RC_OK, "caps");
		processLayoutEvents(disp);
		require(sends == 0, "initial layout waits after display capabilities");
		SDL_Delay(1100);
		require(sends == 1, "timer sends initial layout after capabilities delay");
		SDL_Delay(600);
		ActivatedEventArgs activated{};
		EventArgsInit(&activated, "test");
		activated.firstActivation = TRUE;
		require(PubSub_OnActivated(context->pubSub, context, &activated) >= 0, "first activation");
		processLayoutEvents(disp);
		require(sends == 1, "matching desktop suppresses duplicate");
		auto window = sdl.getFirstWindow();
		require(window != nullptr, "window exists");
		expectedWidth = 1800;
		expectedHeight = 1100;
		require(window->resize({ 1800, 1100 }), "enlarge login window");
		require(SDL_SyncWindow(window->window()), "sync window");
		SDL_FlushEvents(SDL_EVENT_WINDOW_FIRST, SDL_EVENT_WINDOW_LAST);
		SDL_Delay(600);
		SDL_WindowEvent resized{};
		resized.type = SDL_EVENT_WINDOW_RESIZED;
		resized.windowID = window->id();
		require(disp.handleEvent(resized), "login resize");
		SDL_Delay(1100);
		require(sends >= 2, "larger login resolution sent");
		serverSize(settings, 1800, 1100);
		require(disp.uninit(&channel), "old channel teardown");
		monitor.width = 1024;
		monitor.height = 768;
		require(freerdp_settings_set_monitor_def_array_sorted(settings, &monitor, 1),
		        "new channel stale monitor settings");
		serverSize(settings, 1024, 768);
		require(disp.init(&channel), "new channel init");
		const int previous = sends;
		require(channel.DisplayControlCaps(&channel, 1, 4096, 4096) == CHANNEL_RC_OK,
		        "new channel caps");
		processLayoutEvents(disp);
		require(sends == previous, "new channel also waits after display capabilities");
		SDL_Delay(1100);
		require(sends > previous, "new channel also uses actual window size");
		require(disp.uninit(&channel), "uninit");
		sdl.cleanup();
	}
	instance->PostDisconnect = nullptr;
	instance->PostFinalDisconnect = nullptr;
	freerdp_context_free(instance);
	freerdp_free(instance);
	SDL_Quit();
	std::puts("PASS: channel recreation restores cached window layout with stale monitor settings");
}
