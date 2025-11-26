#pragma once

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include "../components/components.hpp"

// Per-instance data structure matching shader layout
struct SpriteInstance {
	float posX, posY;		// location 2: aObjPos (vec2)
	float scale;			// location 3: aObjScale (float)
	float u, v, w, h;		// location 4: aUVRect (vec4)
	float r, g, b, a;		// location 5: aColor (vec4)
};

class RenderSystem {
public:
	void init(const nlohmann::json& config);
	void update(entt::registry& registry);
	
	// Set world dimensions for border rendering
	void SetWorldBounds(int width, int height);
	
	const std::vector<Color>& GetFactionColors() const { return _faction_colors; }
	
private:
	void initLinePipeline();
	void renderWorldBorder(const Vec2& camOffset, float camZoom);

	unsigned int _vao = 0;
	unsigned int _vbo = 0;
	unsigned int _shader_program = 0;
	unsigned int _atlas_texture = 0;
	unsigned int _terrain_texture = 0;
	
	// Hardware instancing resources
	unsigned int _instanceVBO = 0;
	std::vector<SpriteInstance> _batchBuffer;
	
	// Line rendering resources
	unsigned int _line_vao = 0;
	unsigned int _line_vbo = 0;
	unsigned int _line_shader_program = 0;
	
	// World bounds
	int _world_width = 0;
	int _world_height = 0;
	
	// World border color
	Color _border_color = {0.0f, 0.6f, 0.0f, 1.0f};
	
	int _tile_size = 32;
	float _unit_size = 32.0f;

	std::vector<Color> _faction_colors;
	std::vector<UVRect> _unitUVs;
};
