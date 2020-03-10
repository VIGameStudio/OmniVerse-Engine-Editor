#include "editor.hpp"

#include <ove/editor/imgui_impl.hpp>
#include <imgui_internal.h>

using namespace ove::core;
using namespace ove::system;
using namespace ove::framework;
using namespace ove::editor;

void editor_t::init()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	detail::ImGui_Impl_Init(&m_window);
}

void editor_t::update(f32 dt)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse || io.WantCaptureKeyboard)
	{
		return;
	}

	m_game.update(dt);
}

void editor_t::draw(f32 dt)
{
	m_game.draw(dt);

	// Start the Dear ImGui frame
	detail::ImGui_Impl_NewFrame(dt, m_window);
	ImGui::NewFrame();

	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", NULL, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", NULL, false))
			{
				// TODO
			}
			if (ImGui::MenuItem("Open", NULL, false))
			{
				// TODO
			}
			if (ImGui::MenuItem("Save", NULL, false))
			{
				// TODO
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit", NULL, false))
			{
				m_window.close();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Cut", NULL, false))
			{
				// TODO
			}
			if (ImGui::MenuItem("Copy", NULL, false))
			{
				// TODO
			}
			if (ImGui::MenuItem("Paste", NULL, false))
			{
				// TODO
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Undo", NULL, false))
			{
				// TODO
			}
			if (ImGui::MenuItem("Redo", NULL, false))
			{
				// TODO
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Toggle Editor", NULL, false))
			{
				m_showViews = !m_showViews;
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("About", NULL, false))
			{
				// TODO
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	ImGui::End();

	if (m_showViews)
	{
		m_assetsView.draw(dt);
		m_consoleView.draw(dt);
		m_hierarchyView.draw(dt);
		m_inspectorView.draw(dt);
	}

	// Rendering
	ImGui::Render();
	detail::ImGui_Impl_RenderDrawData(ImGui::GetDrawData());
}

void editor_t::clean()
{
	// Cleanup
	detail::ImGui_Impl_Shutdown();
	ImGui::DestroyContext();
}

void editor_t::onWindowScroll(window_t* pWin, f64 dx, f64 dy)
{
	detail::ImGui_Impl_ScrollCallback(pWin, dx, dy);
}

void editor_t::onWindowKey(window_t* pWin, u8 key, bool pressed)
{
	detail::ImGui_Impl_KeyCallback(pWin, key, pressed);
}

void editor_t::onWindowChar(window_t* pWin, u32 c)
{
	detail::ImGui_Impl_CharCallback(pWin, c);
}