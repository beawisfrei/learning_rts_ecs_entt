#include <iostream>
#include <SDL3/SDL.h>
#include "utils/gl_loader.hpp"
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <entt/entt.hpp>

#include "components/components.hpp"
#include "systems/render_system.hpp"
#include "systems/input_system.hpp"
#include "systems/gameplay_system.hpp"
#include "utils/resource_loader.hpp"
#include "utils/spatial_grid.hpp"
#include "utils/unit_factory.hpp"

int main(int argc, char* argv[]) {
	// Init SDL
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow("RTS ECS Example", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!window) {
		std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	if (!glContext) {
		std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}
	SDL_GL_MakeCurrent(window, glContext);
	SDL_GL_SetSwapInterval(1); // VSync

	if (!load_gl_functions()) {
		std::cerr << "Failed to initialize OpenGL functions" << std::endl;
		return 1;
	}

	// Init ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplSDL3_InitForOpenGL(window, glContext);
	ImGui_ImplOpenGL3_Init("#version 330");

	// ECS Setup
	entt::registry registry;
	
	// Load Config
	nlohmann::json config;
	if (!ResourceLoader::load_config("data/config.json", config)) {
		std::cerr << "Failed to load config" << std::endl;
		return 1;
	}
	
	// Systems
	RenderSystem renderSystem;
	InputSystem inputSystem;
	SpatialGrid spatialGrid(registry);
	UnitFactory unitFactory(config);
	GameplaySystem gameplaySystem(spatialGrid);

	renderSystem.init(config);

	// Create Camera
	auto cameraEntity = registry.create();
	registry.emplace<Camera>(cameraEntity, Vec2{0.0f, 0.0f}, 1.0f);
	registry.emplace<MainCamera>(cameraEntity);

	// Create some test units using the factory
	unitFactory.spawn_unit(registry, UnitType::Footman, 0, Vec2{-10.0f, 0.0f});
	unitFactory.spawn_unit(registry, UnitType::Footman, 0, Vec2{-10.0f, 5.0f});
	unitFactory.spawn_unit(registry, UnitType::Archer, 0, Vec2{-10.0f, -5.0f});
	
	unitFactory.spawn_unit(registry, UnitType::Footman, 1, Vec2{10.0f, 0.0f});
	unitFactory.spawn_unit(registry, UnitType::Archer, 1, Vec2{10.0f, 5.0f});
	unitFactory.spawn_unit(registry, UnitType::Ballista, 1, Vec2{10.0f, -5.0f});
	
	unitFactory.spawn_unit(registry, UnitType::Healer, 0, Vec2{-15.0f, 0.0f});

	bool running = true;
	SDL_Event event;
	
	// Delta time tracking
	Uint64 last_time = SDL_GetPerformanceCounter();
	float dt = 0.016f; // Default to ~60fps
	
	// UI state for spawning
	static int spawn_type_idx = 0;
	static int spawn_faction = 0;
	static int spawn_count = 10;
	
	while (running) {
		// Calculate delta time
		Uint64 current_time = SDL_GetPerformanceCounter();
		dt = (float)(current_time - last_time) / (float)SDL_GetPerformanceFrequency();
		last_time = current_time;
		
		// Cap dt to prevent huge jumps
		if (dt > 0.1f) dt = 0.1f;
		
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			inputSystem.process_event(event);
		}

		// Update spawn parameters
		inputSystem.set_spawn_params(static_cast<UnitType>(spawn_type_idx), spawn_faction, spawn_count);

		// Update Systems
		inputSystem.update(registry, spatialGrid, unitFactory, dt);
		gameplaySystem.update(registry, dt);

		// UI Frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Debug");
		ImGui::Text("Application Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("Delta Time: %.3f ms", dt * 1000.0f);
		
		ImGui::Separator();
		
		auto camView = registry.view<Camera, MainCamera>();
		for(auto e : camView) {
			auto& c = camView.get<Camera>(e);
			ImGui::DragFloat2("Cam Offset", &c.offset.x);
			ImGui::DragFloat("Cam Zoom", &c.zoom, 0.1f, 0.1f, 50.0f);
		}
		
		ImGui::Separator();
		ImGui::Text("Spawn Settings");
		const char* unit_types[] = {"Footman", "Archer", "Ballista", "Healer"};
		ImGui::Combo("Unit Type", &spawn_type_idx, unit_types, 4);
		ImGui::SliderInt("Faction", &spawn_faction, 0, 7);
		ImGui::SliderInt("Count", &spawn_count, 1, 100);
		ImGui::Text("Hold S + Drag to spawn");
		ImGui::Text("Hold D + Drag to delete");
		
		ImGui::Separator();
		ImGui::Text("Unit Counts:");
		
		// Count units by type and faction
		int footman_count[8] = {0};
		int archer_count[8] = {0};
		int ballista_count[8] = {0};
		int healer_count[8] = {0};
		int selected_count = 0;
		int projectile_count = 0;
		
		auto unit_view = registry.view<Unit, Faction>();
		for (auto entity : unit_view) {
			const auto& unit = unit_view.get<Unit>(entity);
			const auto& faction = unit_view.get<Faction>(entity);
			
			// Skip projectiles (they have Unit component but shouldn't be counted)
			if (registry.all_of<Projectile>(entity)) {
				projectile_count++;
				continue;
			}
			
			int f = faction.id;
			if (f >= 0 && f < 8) {
				switch (unit.type) {
					case UnitType::Footman: footman_count[f]++; break;
					case UnitType::Archer: archer_count[f]++; break;
					case UnitType::Ballista: ballista_count[f]++; break;
					case UnitType::Healer: healer_count[f]++; break;
				}
			}
			
			if (registry.all_of<Selected>(entity)) {
				selected_count++;
			}
		}
		
		for (int f = 0; f < 8; ++f) {
			int total = footman_count[f] + archer_count[f] + ballista_count[f] + healer_count[f];
			if (total > 0) {
				ImGui::Text("Faction %d: F:%d A:%d B:%d H:%d", f, footman_count[f], archer_count[f], ballista_count[f], healer_count[f]);
			}
		}
		ImGui::Text("Selected: %d", selected_count);
		ImGui::Text("Projectiles: %d", projectile_count);
		
		ImGui::End();
		
		// Draw selection rectangle
		if (inputSystem.is_selecting()) {
			ImGuiIO& io = ImGui::GetIO();
			ImDrawList* draw_list = ImGui::GetForegroundDrawList();
			
			Vec2 start = inputSystem.get_selection_start();
			Vec2 end = inputSystem.get_selection_end();
			
			// Convert world coords to screen coords (reverse of shader transform)
			// Get camera for conversion
			Camera cam = {Vec2{0.0f, 0.0f}, 1.0f};
			for(auto e : camView) {
				cam = camView.get<Camera>(e);
				break;
			}
			
			auto world_to_screen = [&](Vec2 world) -> ImVec2 {
				// worldPos = (pos - offset) * zoom
				float world_x = (world.x - cam.offset.x) * cam.zoom;
				float world_y = (world.y - cam.offset.y) * cam.zoom;
				
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

		ImGui::Render();

		// Render
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		renderSystem.update(registry);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DestroyContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
