#include "assets_view.hpp"

#include <imgui.h>

using namespace ove::core;
using namespace ove::editor;

void assets_view_t::draw(f32 dt)
{
	ImGui::Begin("Assets", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	ImGui::End();
}