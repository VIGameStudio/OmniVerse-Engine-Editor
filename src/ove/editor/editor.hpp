#pragma once

#include <imgui.h>

#include <ove/core/util/types.hpp>
#include <ove/system/window.hpp>
#include <ove/framework/app.hpp>

#include <ove/framework/game.hpp>
#include <ove/editor/assets_view.hpp>
#include <ove/editor/console_view.hpp>
#include <ove/editor/hierarchy_view.hpp>
#include <ove/editor/inspector_view.hpp>

namespace ove
{
	namespace editor
	{
		struct editor_t
		{
		public:
			editor_t(ove::framework::app_t& app, ove::framework::game_t& game)
				: m_window(app.getWindow())
				, m_renderDevice(app.getRenderDevice())
				, m_game(game)
			{}

			void init();
			void update(ove::core::f32 dt);
			void draw(ove::core::f32 dt);
			void clean();

		public:
			void onWindowScroll(ove::system::window_t* pWin, ove::core::f64 dx, ove::core::f64 dy);
			void onWindowKey(ove::system::window_t* pWin, ove::core::u8 key, bool pressed);
			void onWindowChar(ove::system::window_t* pWin, ove::core::u32 c);

		private:
			ove::system::window_t& m_window;
			ove::graphics::render_device_t& m_renderDevice;

			ove::framework::game_t& m_game;

			bool m_showViews = true;
			assets_view_t m_assetsView;
			console_view_t m_consoleView;
			hierarchy_view_t m_hierarchyView;
			inspector_view_t m_inspectorView;
		};
	}
}