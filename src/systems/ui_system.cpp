#include "ui_system.hpp"
#include "input_system.hpp"
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

void UISystem::Render(World& world, InputSystem& inputSystem, float dt) {
	// UI Frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	renderDebugWindow(world, dt);
	renderSelectionRect(world, inputSystem);

	ImGui::Render();
}

void UISystem::renderDebugWindow(World& world, float dt) {
	ImGui::Begin("Debug");
	ImGui::Text("Application Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Text("Delta Time: %.3f ms", dt * 1000.0f);
	
	ImGui::Separator();
	
	// Camera controls
	Camera* cam = world.GetCamera();
	if (cam) {
		ImGui::DragFloat2("Cam Offset", &cam->offset.x);
		ImGui::DragFloat("Cam Zoom", &cam->zoom, 0.1f, 0.1f, 50.0f);
	}
	
	ImGui::Separator();
	ImGui::Text("Spawn Settings");
	const char* unit_types[] = {"Footman", "Archer", "Ballista", "Healer"};
	ImGui::Combo("Unit Type", &_spawnTypeIdx, unit_types, 4);
	ImGui::SliderInt("Faction", &_spawnFaction, 0, 7);
	ImGui::SliderInt("Count", &_spawnCount, 1, 100);
	ImGui::Text("Hold S + Drag to spawn");
	ImGui::Text("Hold D + Drag to delete");
	
	ImGui::Separator();
	ImGui::Text("Unit Counts:");
	
	// Get unit counts from world
	UnitCountData counts = world.GetUnitCounts();
	
	for (int f = 0; f < 8; ++f) {
		int total = counts.footmanCount[f] + counts.archerCount[f] + counts.ballistaCount[f] + counts.healerCount[f];
		if (total > 0) {
			ImGui::Text("Faction %d: F:%d A:%d B:%d H:%d", f, counts.footmanCount[f], counts.archerCount[f], counts.ballistaCount[f], counts.healerCount[f]);
		}
	}
	ImGui::Text("Selected: %d", counts.selectedCount);
	ImGui::Text("Projectiles: %d", counts.projectileCount);
	
	ImGui::End();
}

void UISystem::renderSelectionRect(World& world, InputSystem& inputSystem) {
	if (!inputSystem.is_selecting()) {
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetForegroundDrawList();
	
	Vec2 start = inputSystem.get_selection_start();
	Vec2 end = inputSystem.get_selection_end();
	
	// Get camera for conversion
	Camera* cam = world.GetCamera();
	if (!cam) {
		return;
	}
	
	auto world_to_screen = [&](Vec2 world) -> ImVec2 {
		// worldPos = (pos - offset) * zoom
		Vec2 world_pos = (world - cam->offset) * cam->zoom;
		float world_x = world_pos.x;
		float world_y = world_pos.y;
		
		// ndc = worldPos / vec2(640, 360)
		float ndc_x = world_x / 640.0f;
		float ndc_y = world_y / 360.0f;
		
		// screen = (ndc + 1) / 2 * screen_size
		float screen_x = (ndc_x + 1.0f) * 0.5f * io.DisplaySize.x;
		float screen_y = (1.0f - ndc_y) * 0.5f * io.DisplaySize.y; // Y flipped
		
		return ImVec2(screen_x, screen_y);
	};
	
	ImVec2 p1 = world_to_screen(start);
	ImVec2 p2 = world_to_screen(end);
	
	draw_list->AddRect(p1, p2, IM_COL32(0, 255, 0, 255), 0.0f, 0, 2.0f);
}

