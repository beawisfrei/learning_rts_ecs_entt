#include <iostream>
#include <SDL3/SDL.h>
#include "utils/gl_loader.hpp"
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include "world/world.hpp"
#include "systems/input_system.hpp"
#include "systems/ui_system.hpp"
#include "utils/resource_loader.hpp"
#include "utils/time_controller.hpp"

int main(int argc, char* argv[]) {
	// Load Config first (needed for window dimensions)
	nlohmann::json config;
	if (!ResourceLoader::load_config("data/config.json", config)) {
		std::cerr << "Failed to load config" << std::endl;
		return 1;
	}
	
	// Extract screen dimensions from config
	int screen_width = config["global"].value("screen_width", 1280);
	int screen_height = config["global"].value("screen_height", 720);
	
	// Init SDL
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow("RTS ECS Example", screen_width, screen_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
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
	
	// Create game world and systems
	World world;
	if (!world.Initialize(config)) {
		std::cerr << "Failed to initialize world" << std::endl;
		return 1;
	}

	// Create some initial test units
	world.SpawnUnit(UnitType::Footman, 0, Vec2{-10.0f, 0.0f});
	world.SpawnUnit(UnitType::Footman, 0, Vec2{-10.0f, 5.0f});
	world.SpawnUnit(UnitType::Archer, 0, Vec2{-10.0f, -5.0f});
	
	world.SpawnUnit(UnitType::Footman, 1, Vec2{10.0f, 0.0f});
	world.SpawnUnit(UnitType::Archer, 1, Vec2{10.0f, 5.0f});
	world.SpawnUnit(UnitType::Ballista, 1, Vec2{10.0f, -5.0f});
	
	world.SpawnUnit(UnitType::Healer, 0, Vec2{-15.0f, 0.0f});
	
	InputSystem inputSystem;
	inputSystem.set_screen_dimensions(screen_width, screen_height);
	UISystem uiSystem;
	TimeController timeController;

	bool running = true;
	SDL_Event event;
	
	while (running) {
		// Update time controller and get delta time
		timeController.Update();
		float dt = timeController.GetDeltaTime();
		
		// Process events
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			inputSystem.process_event(event);
		}

		// Update spawn parameters from UI
		inputSystem.set_spawn_params(uiSystem.GetSpawnType(), uiSystem.GetSpawnFaction(), uiSystem.GetSpawnCount());

		// Update systems
		inputSystem.update(world, dt);
		world.Update(dt);

		// Render world
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		world.Render();

		// Render UI
		uiSystem.Render(world, inputSystem, dt, timeController);
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

