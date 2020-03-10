#include "console_view.hpp"

#include <imgui.h>

using namespace ove::core;
using namespace ove::editor;

void console_view_t::draw(f32 dt)
{
	ImGui::Begin("Console", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	ImGui::End();
}