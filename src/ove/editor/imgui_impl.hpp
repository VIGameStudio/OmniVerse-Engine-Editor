#pragma once

#include <imgui.h>

#include <ove/core/util/types.hpp>
#include <ove/system/window.hpp>

namespace ove
{
	namespace editor
	{
		namespace detail
		{
			// Backend API
			IMGUI_IMPL_API bool ImGui_Impl_Init(ove::system::window_t* pWindow);
			IMGUI_IMPL_API void ImGui_Impl_Shutdown();
			IMGUI_IMPL_API void ImGui_Impl_NewFrame(ove::core::f32 dt, ove::system::window_t& window);
			IMGUI_IMPL_API void ImGui_Impl_RenderDrawData(ImDrawData* draw_data);

			// (Optional) Called by Init/NewFrame/Shutdown
			IMGUI_IMPL_API bool ImGui_Impl_CreateFontsTexture();
			IMGUI_IMPL_API void ImGui_Impl_DestroyFontsTexture();
			IMGUI_IMPL_API bool ImGui_Impl_CreateDeviceObjects();
			IMGUI_IMPL_API void ImGui_Impl_DestroyDeviceObjects();

			IMGUI_IMPL_API void ImGui_Impl_ScrollCallback(ove::system::window_t* pWindow, ove::core::f32 dx, ove::core::f32 dy);
			IMGUI_IMPL_API void ImGui_Impl_KeyCallback(ove::system::window_t* pWindow, ove::core::u8 key, bool pressed);
			IMGUI_IMPL_API void ImGui_Impl_CharCallback(ove::system::window_t* pWindow, ove::core::u32 c);
		}
	}
}