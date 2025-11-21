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
#include "utils/resource_loader.hpp"

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
	
	// Systems
	RenderSystem renderSystem;
	InputSystem inputSystem;

	// Load Config
	nlohmann::json config;
	if (!ResourceLoader::load_config("data/config.json", config)) {
		std::cerr << "Failed to load config" << std::endl;
		return 1;
	}

	renderSystem.init(config);

	// Create Camera
	auto cameraEntity = registry.create();
	registry.emplace<Camera>(cameraEntity, Vec2{0.0f, 0.0f}, 1.0f);
	registry.emplace<MainCamera>(cameraEntity);

	// Create Units
	// 1. Footman, Faction 0
	auto unit1 = registry.create();
	registry.emplace<Position>(unit1, Vec2{0.0f, 0.0f});
	registry.emplace<Unit>(unit1, UnitType::Footman, 0);

	// 2. Archer, Faction 1
	auto unit2 = registry.create();
	registry.emplace<Position>(unit2, Vec2{50.0f, 0.0f});
	registry.emplace<Unit>(unit2, UnitType::Archer, 1);

	// 3. Ballista, Faction 2
	auto unit3 = registry.create();
	registry.emplace<Position>(unit3, Vec2{0.0f, 50.0f});
	registry.emplace<Unit>(unit3, UnitType::Ballista, 2);

	// 4. Healer, Faction 3
	auto unit4 = registry.create();
	registry.emplace<Position>(unit4, Vec2{50.0f, 50.0f});
	registry.emplace<Unit>(unit4, UnitType::Healer, 3);

	bool running = true;
	SDL_Event event;
	
	while (running) {
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			inputSystem.process_event(event);
		}

		// Update
		inputSystem.update(registry);

		// UI Frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Debug");
		ImGui::Text("Application Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		
		auto camView = registry.view<Camera, MainCamera>();
		for(auto e : camView) {
			auto& c = camView.get<Camera>(e);
			ImGui::DragFloat2("Cam Offset", &c.offset.x);
			ImGui::DragFloat("Cam Zoom", &c.zoom, 0.1f, 0.1f, 50.0f);
		}
		
		ImGui::End();

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
