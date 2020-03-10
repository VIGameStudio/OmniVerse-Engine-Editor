#include "inspector_view.hpp"

#include <imgui.h>

using namespace ove::core;
using namespace ove::editor;

void inspector_view_t::draw(f32 dt)
{
	ImGui::Begin("Inspector", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	ImGui::End();
}