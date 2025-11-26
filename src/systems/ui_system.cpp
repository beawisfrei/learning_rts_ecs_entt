#include "ui_system.hpp"
#include "input_system.hpp"
#include "../utils/time_controller.hpp"
#include "../utils/profiler.hpp"
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <cstring>
#include <cmath>
#include <sstream>
#include <iomanip>

void UISystem::Render(World& world, InputSystem& inputSystem, float dt, TimeController& timeController) {
	ZoneScopedN("UISystem::Render");
	// UI Frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	renderDebugWindow(world, dt, timeController);
	renderSelectionWindow(world);
	renderSelectionRect(world, inputSystem);

	ImGui::Render();
}

void UISystem::renderDebugWindow(World& world, float dt, TimeController& timeController) {
	ZoneScopedN("UISystem::renderDebugWindow");
	ImGui::Begin("Debug");
	ImGui::Text("Application Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Text("Delta Time: %.3f ms", dt * 1000.0f);
	
	// Time control: Play/Pause button and speed slider
	const float speedValues[] = {1.0f/20.0f, 1.0f/10.0f, 1.0f/5.0f, 1.0f/3.0f, 1.0f/2.0f, 1.0f, 2.0f, 3.0f, 5.0f, 10.0f, 20.0f};
	const int speedCount = 11;
	
	// Find current speed index
	int currentSpeedIndex = 5; // Default to 1.0 (index 5)
	float currentSpeed = timeController.GetSpeedCoefficient();
	for (int i = 0; i < speedCount; ++i) {
		if (std::abs(speedValues[i] - currentSpeed) < 0.001f) {
			currentSpeedIndex = i;
			break;
		}
	}
	
	// Play/Pause button
	if (ImGui::Button(timeController.IsPaused() ? "Play" : "Pause")) {
		timeController.SetPaused(!timeController.IsPaused());
	}
	
	ImGui::SameLine();
	
	// Speed slider
	int speedSliderValue = currentSpeedIndex;
	if (ImGui::SliderInt("Speed", &speedSliderValue, 0, speedCount - 1)) {
		timeController.SetSpeedCoefficient(speedValues[speedSliderValue]);
	}
	
	// Display current speed value as text
	ImGui::SameLine();
	ImGui::Text("(%.2fx)", speedValues[speedSliderValue]);
	
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
	ImGui::SliderInt("Faction", &_spawnFaction, 0, MAX_FACTIONS - 1);
	ImGui::SliderInt("Count", &_spawnCount, 1, 1000);
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
	
	ImGui::Separator();
	ImGui::Text("Save/Load Game");
	
	// File path input
	char filePathBuffer[256];
	std::strncpy(filePathBuffer, _saveFilePath.c_str(), sizeof(filePathBuffer) - 1);
	filePathBuffer[sizeof(filePathBuffer) - 1] = '\0';
	
	if (ImGui::InputText("File Path", filePathBuffer, sizeof(filePathBuffer))) {
		_saveFilePath = std::string(filePathBuffer);
	}
	
	// Save and Load buttons
	if (ImGui::Button("Save Game")) {
		if (world.SaveGame(_saveFilePath)) {
			_saveLoadStatus = "Game saved successfully!";
		} else {
			_saveLoadStatus = "Error: Failed to save game.";
		}
	}
	
	ImGui::SameLine();
	
	if (ImGui::Button("Load Game")) {
		if (world.LoadGame(_saveFilePath)) {
			_saveLoadStatus = "Game loaded successfully!";
		} else {
			_saveLoadStatus = "Error: Failed to load game.";
		}
	}
	
	// Status message
	if (!_saveLoadStatus.empty()) {
		ImVec4 color = (_saveLoadStatus.find("Error") != std::string::npos) 
			? ImVec4(1.0f, 0.0f, 0.0f, 1.0f)  // Red for errors
			: ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for success
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::Text("%s", _saveLoadStatus.c_str());
		ImGui::PopStyleColor();
	}
	
	ImGui::End();
}

void UISystem::renderSelectionRect(World& world, InputSystem& inputSystem) {
	ZoneScopedN("UISystem::renderSelectionRect");
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

void UISystem::renderSelectionWindow(World& world) {
	ZoneScopedN("UISystem::renderSelectionWindow");
	entt::registry& registry = world.GetRegistry();
	const std::vector<Color>& factionColors = world.GetFactionColors();
	
	// Get all selected units
	auto selectedView = registry.view<Selected, Unit>();
	
	// Collect first 50 selected units
	std::vector<entt::entity> selectedUnits;
	selectedUnits.reserve(50);
	for (auto entity : selectedView) {
		if (selectedUnits.size() >= 50) {
			break;
		}
		selectedUnits.push_back(entity);
	}
	
	// Resize info array
	_selectionInfo.resize(selectedUnits.size());
	
	// Helper function to format float with 1 decimal place
	auto formatFloat = [](float value) -> std::string {
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(1) << value;
		return oss.str();
	};
	
	// Populate info for each unit
	for (size_t i = 0; i < selectedUnits.size(); ++i) {
		entt::entity entity = selectedUnits[i];
		UnitInfoLine& info = _selectionInfo[i];
		
		// Get unit and faction
		const auto& unit = registry.get<Unit>(entity);
		int factionIdx = unit.faction;
		
		// Get faction color
		if (factionIdx >= 0 && factionIdx < static_cast<int>(factionColors.size())) {
			const Color& color = factionColors[factionIdx];
			info.color = ImVec4(color.r, color.g, color.b, color.a);
		} else {
			info.color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
		}
		
		// Build info string
		std::ostringstream oss;
		
		// Health component
		if (registry.all_of<Health>(entity)) {
			const auto& health = registry.get<Health>(entity);
			oss << "H:" << static_cast<int>(health.current)
			    << ", M:" << static_cast<int>(health.max)
			    << ", S:" << static_cast<int>(health.shield)
			    << ", ";
		}
		
		// Unit component
		oss << "F:" << unit.faction
		    << ", T:" << static_cast<int>(unit.type)
		    << ", ";
		
		// Movement component
		if (registry.all_of<Movement>(entity)) {
			const auto& movement = registry.get<Movement>(entity);
			oss << "Sp:" << formatFloat(movement.speed) << ", ";
		}
		
		// DirectDamage component
		if (registry.all_of<DirectDamage>(entity)) {
			const auto& damage = registry.get<DirectDamage>(entity);
			oss << "D:" << formatFloat(damage.damage)
			    << ", R:" << formatFloat(damage.range)
			    << ", C:" << formatFloat(damage.cooldown)
			    << ", ";
		}
		
		// ProjectileEmitter component
		if (registry.all_of<ProjectileEmitter>(entity)) {
			const auto& emitter = registry.get<ProjectileEmitter>(entity);
			oss << "D:" << formatFloat(emitter.damage)
			    << ", R:" << formatFloat(emitter.range)
			    << ", C:" << formatFloat(emitter.cooldown)
			    << ", PS:" << formatFloat(emitter.projectile_speed)
			    << ", ";
		}
		
		// Healer component
		if (registry.all_of<Healer>(entity)) {
			const auto& healer = registry.get<Healer>(entity);
			oss << "He:" << formatFloat(healer.heal_amount)
			    << ", R:" << formatFloat(healer.range)
			    << ", C:" << formatFloat(healer.cooldown)
			    << ", ";
		}
		
		info.text = oss.str();
		
		// Remove trailing comma and space
		if (!info.text.empty() && info.text.size() >= 2) {
			if (info.text.substr(info.text.size() - 2) == ", ") {
				info.text = info.text.substr(0, info.text.size() - 2);
			}
		}
	}
	
	// Create ImGui window
	ImGui::Begin("Selection");
	
	// Scrollable child area
	ImGui::BeginChild("SelectionList", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	
	// Display each unit info line
	for (const auto& info : _selectionInfo) {
		ImGui::PushStyleColor(ImGuiCol_Text, info.color);
		ImGui::Text("%s", info.text.c_str());
		ImGui::PopStyleColor();
	}
	
	ImGui::EndChild();
	ImGui::End();
}

