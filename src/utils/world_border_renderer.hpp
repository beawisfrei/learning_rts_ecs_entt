#pragma once

#include "vec2.hpp"

// Color structure for border rendering
struct BorderColor {
	float r, g, b, a;
};

// WorldBorderRenderer - renders world boundary as a rectangle using triangles
// Uses 4 thin quads (2 triangles each = 8 triangles total) for the border lines
class WorldBorderRenderer {
public:
	void Init();
	void Shutdown();
	
	// Set world dimensions and border thickness
	void SetWorldBounds(float width, float height, float thickness = 2.0f);
	
	// Set border color
	void SetColor(float r, float g, float b, float a = 1.0f);
	void SetColor(const BorderColor& color);
	
	// Render the world border
	void Render(const Vec2& camOffset, float camZoom);

private:
	void rebuildVertexBuffer();

	unsigned int _vao = 0;
	unsigned int _vbo = 0;
	unsigned int _shaderProgram = 0;
	
	float _worldWidth = 0.0f;
	float _worldHeight = 0.0f;
	float _borderThickness = 2.0f;
	
	BorderColor _color = {0.0f, 0.6f, 0.0f, 1.0f};
	
	bool _needsRebuild = true;
};

