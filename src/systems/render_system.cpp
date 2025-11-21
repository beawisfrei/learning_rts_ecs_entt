#include "render_system.hpp"
#include "../utils/gl_loader.hpp"
#include "../utils/resource_loader.hpp"
#include "../components/components.hpp"
#include <iostream>
#include <array>

// Shader supporting textures and colors
const char* vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform vec2 uOffset;
uniform float uZoom;
uniform vec2 uObjPos;
uniform vec2 uObjScale; 

// UV rect: x, y, w, h (already normalized [0,1])
uniform vec4 uUVRect;

out vec2 TexCoord;

void main() {
    // World position of the vertex
    vec2 scaledPos = aPos * uObjScale;
    vec2 worldPos = (scaledPos + uObjPos - uOffset) * uZoom;
    
    // Screen space (NDC)
    vec2 ndc = worldPos / vec2(640.0, 360.0); 
    gl_Position = vec4(ndc, 0.0, 1.0);
    
    // Calculate TexCoord based on UV Rect
    // aTexCoord is [0,1] for the full quad.
    // We map 0->uUVRect.x, 1->uUVRect.x + uUVRect.w
    TexCoord.x = uUVRect.x + (aTexCoord.x * uUVRect.z); // z is w
    TexCoord.y = uUVRect.y + (aTexCoord.y * uUVRect.w); // w is h
}
)";

const char* fragment_shader_src = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D uTexture;
uniform vec4 uColor;

void main() {
    vec4 texColor = texture(uTexture, TexCoord);
    // Simple tinting: Texture Color * Faction Color
    // FragColor = texColor * uColor;
    // For now, just show the faction color if texture is white/gray, or tint it.
    // Since we don't have a real texture yet (or might have dummy one), let's mix.
    // If texColor is white, it becomes uColor.
    
    // Use uColor directly if texture is not important yet, or multiply.
    // FragColor = uColor; 
    // Let's try tinting
    FragColor = texColor * uColor;
    
    // Alpha discard for transparency if needed
    if (FragColor.a < 0.1) discard;
}
)";

void RenderSystem::init(const nlohmann::json& config) {
	// Load Config params
	if(config.contains("global")) {
		_tile_size = config["global"].value("tile_size", 32);
        _unit_size = config["global"].value("unit_size", 2.0f);
	}

	// Load Textures
	_atlas_texture = ResourceLoader::load_texture("data/unit_atlas.png");
	// _terrain_texture = ResourceLoader::load_texture("data/terrain.png"); // Todo: terrain rendering

	// Compile Shaders
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertex_shader_src, NULL);
	glCompileShader(vertexShader);

	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragment_shader_src, NULL);
	glCompileShader(fragmentShader);

	_shader_program = glCreateProgram();
	glAttachShader(_shader_program, vertexShader);
	glAttachShader(_shader_program, fragmentShader);
	glLinkProgram(_shader_program);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Setup Quad (Standard 1x1 centered, will scale in shader)
	// Vertices: x, y, u, v
	float vertices[] = {
		 -0.5f,  0.5f,  0.0f, 0.0f, // Top-left
		  0.5f,  0.5f,  1.0f, 0.0f, // Top-right
		  0.5f, -0.5f,  1.0f, 1.0f, // Bottom-right
		 -0.5f, -0.5f,  0.0f, 1.0f  // Bottom-left
	};

	glGenVertexArrays(1, &_vao);
	glGenBuffers(1, &_vbo);

	glBindVertexArray(_vao);

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// TexCoord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	glBindVertexArray(0);

	// Init Data Arrays from Config
	// Load Faction Colors
	if (config.contains("factions")) {
		_faction_colors.clear();
		for (const auto& faction : config["factions"]) {
			Color color;
			color.r = faction["color"][0].get<int>() / 255.0f;
			color.g = faction["color"][1].get<int>() / 255.0f;
			color.b = faction["color"][2].get<int>() / 255.0f;
			color.a = faction["color"][3].get<int>() / 255.0f;
			_faction_colors.push_back(color);
		}
	}

	// Load Unit UVs
	if (config.contains("units")) {
		_unitUVs.clear();
		for (const auto& unit : config["units"]) {
			UVRect uv;
			uv.x = unit.value("uv_x", 0.0f);
			uv.y = unit.value("uv_y", 0.0f);
			uv.w = unit.value("uv_w", 0.5f);
			uv.h = unit.value("uv_h", 1.0f);
			_unitUVs.push_back(uv);
		}
	}
}

void RenderSystem::update(entt::registry& registry) {
	glUseProgram(_shader_program);
	glBindVertexArray(_vao);
	
	// Bind Texture
	// Using standard 1.1 function since we link opengl32 now
	::glBindTexture(GL_TEXTURE_2D, _atlas_texture);

	// Get Camera
	auto camView = registry.view<Camera, MainCamera>();
	Vec2 camOffset = {0.0f, 0.0f};
	float camZoom = 1.0f;

	for(auto entity : camView) {
		const auto& cam = camView.get<Camera>(entity);
		camOffset = cam.offset;
		camZoom = cam.zoom;
		break; 
	}

	int offsetLoc = glGetUniformLocation(_shader_program, "uOffset");
	int zoomLoc = glGetUniformLocation(_shader_program, "uZoom");
	int objPosLoc = glGetUniformLocation(_shader_program, "uObjPos");
	int objScaleLoc = glGetUniformLocation(_shader_program, "uObjScale");
	int uvRectLoc = glGetUniformLocation(_shader_program, "uUVRect");
	int colorLoc = glGetUniformLocation(_shader_program, "uColor");

	glUniform2f(offsetLoc, camOffset.x, camOffset.y);
	glUniform1f(zoomLoc, camZoom);

	// Enable Alpha Blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Render all entities with Position and Unit components
	auto view = registry.view<Position, Unit>();
	for (auto entity : view) {
		const auto& pos = view.get<Position>(entity);
		const auto& unit = view.get<Unit>(entity);
		
		// Safety check for indices
		int typeIdx = static_cast<int>(unit.type);
		int factionIdx = unit.faction;

		if (typeIdx < 0 || typeIdx >= _unitUVs.size()) typeIdx = 0;
		if (factionIdx < 0 || factionIdx >= _faction_colors.size()) factionIdx = 0;

		const auto& uv = _unitUVs[typeIdx];
		const auto& color = _faction_colors[factionIdx];

		glUniform2f(objPosLoc, pos.value.x, pos.value.y);
		glUniform2f(objScaleLoc, _unit_size, _unit_size);
		// uUVRect: x, y, w, h
		glUniform4f(uvRectLoc, uv.x, uv.y, uv.w, uv.h);
		
		// Color
		glUniform4f(colorLoc, color.r, color.g, color.b, color.a);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
	
	glBindVertexArray(0);
	glUseProgram(0);
}
