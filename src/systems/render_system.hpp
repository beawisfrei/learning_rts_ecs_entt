#pragma once

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include "../components/components.hpp"

class RenderSystem {
public:
	void init(const nlohmann::json& config);
	void update(entt::registry& registry);
	
private:
	unsigned int _vao = 0;
	unsigned int _vbo = 0;
	unsigned int _shader_program = 0;
	unsigned int _atlas_texture = 0;
	unsigned int _terrain_texture = 0;
	
	int _tile_size = 32;
	float _unit_size = 32.0f;

	std::vector<Color> _faction_colors;
	std::vector<UVRect> _unitUVs;
};
