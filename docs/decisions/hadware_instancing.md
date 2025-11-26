To make this efficient, we need to eliminate the `glUniform` and `glDrawArrays` calls inside the loop. The standard, modern OpenGL approach for this is **Hardware Instancing**.

This technique allows us to:

1.  Store the static geometry (the quad) once.
2.  Pack dynamic data (Position, Color, UVs, Scale) into a `std::vector` on the CPU.
3.  Upload that vector to the GPU in one go (`glBufferSubData` or `glBufferData`).
4.  Issue a **single** `glDrawArraysInstanced` call.

Here is the complete implementation strategy.

### 1\. The Shader Changes

You must update your vertex shader. Instead of using `uniform` for per-object properties, we treat them as **Vertex Attributes**.

**Vertex Shader (`.vert`):**

```glsl
#version 330 core

// 0: The static quad vertex (e.g., 0,0 to 1,1)
layout (location = 0) in vec2 aVertexPos; 

// Per-Instance Attributes (These change per sprite)
layout (location = 1) in vec2 aObjPos;
layout (location = 2) in float aObjScale;
layout (location = 3) in vec4 aUVRect;
layout (location = 4) in vec4 aColor;

// Global Uniforms (Constant for the whole frame)
uniform vec2 uOffset; // Camera Offset
uniform float uZoom;  // Camera Zoom

out vec2 vTexCoord;
out vec4 vColor;

void main() {
    // 1. Calculate world position based on instance pos + scaled vertex
    // Assuming aVertexPos is centered (-0.5 to 0.5) or requires centering logic
    vec2 worldPos = aObjPos + (aVertexPos * aObjScale);

    // 2. Apply Camera Transform
    vec2 screenPos = (worldPos - uOffset) * uZoom;

    // 3. Output position (assuming screenPos is already in NDC or you have a projection matrix)
    // If you have a projection matrix, multiply it here. 
    // For this snippet, I'll pass it straight through assuming your logic handles NDC.
    gl_Position = vec4(screenPos, 0.0, 1.0);

    // 4. Pass Data to Fragment Shader
    // Map 0..1 vertex to UV rect
    // aVertexPos assumed 0..1 or centered. If centered, adjust accordingly.
    // Assuming aVertexPos is 0..1:
    vTexCoord = vec2(
        aUVRect.x + aVertexPos.x * aUVRect.z, // u + x * w
        aUVRect.y + aVertexPos.y * aUVRect.w  // v + y * h
    );
    
    vColor = aColor;
}
```

-----

### 2\. C++ Setup (Initialization)

We need a structure to match the layout locations 1, 2, 3, and 4 in the shader.

**Header / Class Definition:**

```cpp
// This struct matches the Per-Instance attributes in the shader
struct SpriteInstance {
    float posX, posY;       // loc 1
    float scale;            // loc 2
    float u, v, w, h;       // loc 3
    float r, g, b, a;       // loc 4
};

// Add these to your RenderSystem class
GLuint _instanceVBO; 
std::vector<SpriteInstance> _batchBuffer;
```

**Initialization Code (Run once):**

```cpp
void RenderSystem::init() {
    // 1. Setup the Quad Geometry (Static VBO) - Location 0
    // ... existing VBO setup for the quad vertices ...

    // 2. Setup the Instance Buffer (Dynamic VBO)
    glGenBuffers(1, &_instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, _instanceVBO);
    // Allocate simpler buffer size (e.g., max 10k sprites) to start
    glBufferData(GL_ARRAY_BUFFER, 10000 * sizeof(SpriteInstance), nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(_vao);

    // Enable Quad Geometry (Location 0)
    // ... glEnableVertexAttribArray(0); ...

    // --- INSTANCE ATTRIBUTES SETUP ---
    // Note: We bind the _instanceVBO before defining these pointers
    glBindBuffer(GL_ARRAY_BUFFER, _instanceVBO);

    size_t stride = sizeof(SpriteInstance);

    // Location 1: ObjPos (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, posX));
    glVertexAttribDivisor(1, 1); // IMPORTANT: Tell OpenGL this updates once per INSTANCE

    // Location 2: ObjScale (float)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, scale));
    glVertexAttribDivisor(2, 1);

    // Location 3: UVRect (vec4)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, u));
    glVertexAttribDivisor(3, 1);

    // Location 4: Color (vec4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, r));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
}
```

-----

### 3\. The Optimized Render Loop

Here is the rewritten `RenderSystem::update`. It gathers all data into a vector first, uploads it once, and draws once.

```cpp
void RenderSystem::update(entt::registry& registry) {
    // 1. Camera Logic (Unchanged)
    auto camView = registry.view<Camera, MainCamera>();
    Vec2 camOffset = {0.0f, 0.0f};
    float camZoom = 1.0f;

    for(auto entity : camView) {
        const auto& cam = camView.get<Camera>(entity);
        camOffset = cam.offset;
        camZoom = cam.zoom;
        break; 
    }

    renderWorldBorder(camOffset, camZoom);

    // 2. Prepare for Batching
    glUseProgram(_shader_program);
    glBindVertexArray(_vao);
    ::glBindTexture(GL_TEXTURE_2D, _atlas_texture);

    // Set Global Uniforms (Only the ones that don't change per sprite)
    glUniform2f(glGetUniformLocation(_shader_program, "uOffset"), camOffset.x, camOffset.y);
    glUniform1f(glGetUniformLocation(_shader_program, "uZoom"), camZoom);

    // Alpha Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 3. Collect Instance Data
    auto view = registry.view<Position, Unit>();
    
    // Clear the CPU buffer
    _batchBuffer.clear();
    // Reserve memory to avoid reallocations during loop
    _batchBuffer.reserve(view.size_hint()); 

    for (auto entity : view) {
        const auto& pos = view.get<Position>(entity);
        const auto& unit = view.get<Unit>(entity);

        // --- Logic Calculation (Same as before) ---
        int typeIdx = static_cast<int>(unit.type);
        int factionIdx = unit.faction;

        if (typeIdx < 0 || typeIdx >= _unitUVs.size()) typeIdx = 0;
        if (factionIdx < 0 || factionIdx >= _faction_colors.size()) factionIdx = 0;

        const auto& uv = _unitUVs[typeIdx];
        Color color = _faction_colors[factionIdx];

        if (registry.all_of<Selected>(entity)) {
            color.r = (color.r + 1.0f) * 0.5f;
            color.g = (color.g + 1.0f) * 0.5f;
            color.b = (color.b + 1.0f) * 0.5f;
        }

        float size = _unit_size;
        if (registry.all_of<Projectile>(entity)) {
            size = _unit_size * 0.3f;
        }

        // --- Pack Data into Struct ---
        _batchBuffer.push_back({
            pos.value.x, pos.value.y, // Pos
            size,                     // Scale
            uv.x, uv.y, uv.w, uv.h,   // UV
            color.r, color.g, color.b, color.a // Color
        });
    }

    // 4. Upload Data (The only heavy bandwidth operation)
    if (!_batchBuffer.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, _instanceVBO);
        
        // Orphan the buffer (optional but good for drivers) -> pass NULL/size first
        // If your buffer size changes drastically, you might want to specifically check capacity
        glBufferData(GL_ARRAY_BUFFER, _batchBuffer.size() * sizeof(SpriteInstance), _batchBuffer.data(), GL_DYNAMIC_DRAW);
        
        // 5. Single Draw Call
        // Draw 4 vertices (the quad) X number of instances
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(_batchBuffer.size()));
    }

    glBindVertexArray(0);
    glUseProgram(0);
}
```

### Why this is faster

1.  **CPU/Driver Overhead:** Previously, if you had 10,000 units, you made 10,000 calls to `glDrawArrays` and roughly 50,000 calls to `glUniform`. This kills the CPU. The new code has **1** draw call and **2** uniform calls.
2.  **Memory Bandwidth:** You are sending tightly packed structs. We are transferring roughly 44 bytes per unit per frame, which modern PCIe buses can handle easily for millions of units.

### Would you like me to...

Help you implement **Multi-Draw Indirect (MDI)** or **SSBOs**? While Instancing is great (and likely enough here), MDI is the next step up if you eventually need to cull invisible units using a Compute Shader on the GPU.