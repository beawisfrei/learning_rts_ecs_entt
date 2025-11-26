#include "world_border_renderer.hpp"
#include "gl_loader.hpp"
#include "profiler.hpp"
#include <array>

// Vertex shader for border rendering
static const char* border_vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec2 uOffset;
uniform float uZoom;

void main() {
    vec2 worldPos = (aPos - uOffset) * uZoom;
    vec2 ndc = worldPos / vec2(640.0, 360.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
}
)";

// Fragment shader for border rendering
static const char* border_fragment_shader_src = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 uColor;

void main() {
    FragColor = uColor;
}
)";

void WorldBorderRenderer::Init() {
	// Compile vertex shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &border_vertex_shader_src, NULL);
	glCompileShader(vertexShader);

	// Compile fragment shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &border_fragment_shader_src, NULL);
	glCompileShader(fragmentShader);

	// Link shader program
	_shaderProgram = glCreateProgram();
	glAttachShader(_shaderProgram, vertexShader);
	glAttachShader(_shaderProgram, fragmentShader);
	glLinkProgram(_shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Create VAO and VBO
	glGenVertexArrays(1, &_vao);
	glGenBuffers(1, &_vbo);

	glBindVertexArray(_vao);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);

	// Position attribute (vec2)
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void WorldBorderRenderer::Shutdown() {
	if (_vbo != 0) {
		glDeleteBuffers(1, &_vbo);
		_vbo = 0;
	}
	if (_vao != 0) {
		glDeleteVertexArrays(1, &_vao);
		_vao = 0;
	}
	if (_shaderProgram != 0) {
		glDeleteProgram(_shaderProgram);
		_shaderProgram = 0;
	}
}

void WorldBorderRenderer::SetWorldBounds(float width, float height, float thickness) {
	if (_worldWidth != width || _worldHeight != height || _borderThickness != thickness) {
		_worldWidth = width;
		_worldHeight = height;
		_borderThickness = thickness;
		_needsRebuild = true;
	}
}

void WorldBorderRenderer::SetColor(float r, float g, float b, float a) {
	_color = {r, g, b, a};
}

void WorldBorderRenderer::SetColor(const BorderColor& color) {
	_color = color;
}

void WorldBorderRenderer::rebuildVertexBuffer() {
	if (!_needsRebuild) return;
	_needsRebuild = false;

	float w = _worldWidth;
	float h = _worldHeight;
	float t = _borderThickness;

	// Build 4 quads (thin rectangles) for the border lines
	// Each quad = 2 triangles = 6 vertices
	// Total: 4 quads * 6 vertices = 24 vertices
	//
	// Border layout (t = thickness):
	//
	//    +------------------------------------------+
	//    |################## TOP ###################|  (height = t)
	//    +--+------------------------------------+--+
	//    |L |                                    |R |
	//    |E |                                    |I |
	//    |F |                                    |G |
	//    |T |                                    |H |
	//    |  |                                    |T |
	//    +--+------------------------------------+--+
	//    |################# BOTTOM #################|  (height = t)
	//    +------------------------------------------+

	// Using world coordinates where (0,0) is bottom-left
	std::array<float, 48> vertices;

	// Bottom edge: from (0, 0) to (w, t)
	// Triangle 1
	vertices[0] = 0.0f; vertices[1] = 0.0f;      // bottom-left
	vertices[2] = w;    vertices[3] = 0.0f;      // bottom-right
	vertices[4] = w;    vertices[5] = t;         // top-right
	// Triangle 2
	vertices[6] = 0.0f; vertices[7] = 0.0f;      // bottom-left
	vertices[8] = w;    vertices[9] = t;         // top-right
	vertices[10] = 0.0f; vertices[11] = t;       // top-left

	// Top edge: from (0, h-t) to (w, h)
	// Triangle 1
	vertices[12] = 0.0f; vertices[13] = h - t;   // bottom-left
	vertices[14] = w;    vertices[15] = h - t;   // bottom-right
	vertices[16] = w;    vertices[17] = h;       // top-right
	// Triangle 2
	vertices[18] = 0.0f; vertices[19] = h - t;   // bottom-left
	vertices[20] = w;    vertices[21] = h;       // top-right
	vertices[22] = 0.0f; vertices[23] = h;       // top-left

	// Left edge: from (0, t) to (t, h-t)
	// Triangle 1
	vertices[24] = 0.0f; vertices[25] = t;       // bottom-left
	vertices[26] = t;    vertices[27] = t;       // bottom-right
	vertices[28] = t;    vertices[29] = h - t;   // top-right
	// Triangle 2
	vertices[30] = 0.0f; vertices[31] = t;       // bottom-left
	vertices[32] = t;    vertices[33] = h - t;   // top-right
	vertices[34] = 0.0f; vertices[35] = h - t;   // top-left

	// Right edge: from (w-t, t) to (w, h-t)
	// Triangle 1
	vertices[36] = w - t; vertices[37] = t;      // bottom-left
	vertices[38] = w;     vertices[39] = t;      // bottom-right
	vertices[40] = w;     vertices[41] = h - t;  // top-right
	// Triangle 2
	vertices[42] = w - t; vertices[43] = t;      // bottom-left
	vertices[44] = w;     vertices[45] = h - t;  // top-right
	vertices[46] = w - t; vertices[47] = h - t;  // top-left

	// Upload to GPU
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void WorldBorderRenderer::Render(const Vec2& camOffset, float camZoom) {
	ZoneScopedN("WorldBorderRenderer::Render");
	
	if (_worldWidth <= 0 || _worldHeight <= 0) return;

	// Rebuild vertex buffer if needed
	rebuildVertexBuffer();

	glUseProgram(_shaderProgram);
	glBindVertexArray(_vao);

	// Set uniforms
	int offsetLoc = glGetUniformLocation(_shaderProgram, "uOffset");
	int zoomLoc = glGetUniformLocation(_shaderProgram, "uZoom");
	int colorLoc = glGetUniformLocation(_shaderProgram, "uColor");

	glUniform2f(offsetLoc, camOffset.x, camOffset.y);
	glUniform1f(zoomLoc, camZoom);
	glUniform4f(colorLoc, _color.r, _color.g, _color.b, _color.a);

	// Draw all 8 triangles (24 vertices)
	glDrawArrays(GL_TRIANGLES, 0, 24);

	glBindVertexArray(0);
	glUseProgram(0);
}

