# **Software Design Document: RTS EnTT Example**

Version: 1.1

Target Platform: Windows & MacOS

**Language:** C++17

**Build System:** CMake

## **1\. Project Overview**

The goal of this project is to create a lightweight Real-Time Strategy (RTS) example to demonstrate the capabilities of the **EnTT** Entity Component System (ECS). The application focuses on high-performance unit rendering, efficient collision avoidance using **RVO2 (ORCA)**, and a data-driven architecture.

### **1.1. Key Technical Decisions**

* **ECS Architecture:** Use **EnTT** for strict data-oriented design.  
* **Rendering:** **OpenGL** (via Glad/SDL3) for batch rendering of sprites.  
* **Input/Window:** **SDL3** for cross-platform windowing and input events.  
* **Navigation:** **RVO2 Library** for local collision avoidance (Steering). No A\* pathfinding (convex obstacles assumed).  
* **Data Format:** JSON for configuration; PNG for terrain and unit atlases.  
* **Testing:** **Google Test (GTest)** for unit and integration testing.

## **2\. Architecture & Tech Stack**

### **2.1. Dependencies**

All dependencies are managed via CMake FetchContent.

| Library | Purpose |
| :---- | :---- |
| **EnTT** | Core ECS framework (Registry, Views, Groups). |
| **SDL3** | Window creation, Input handling, OpenGL context creation. |
| **Glad** | OpenGL function loader (optional, or use SDL's OpenGL loader). |
| **nlohmann/json** | Parsing config.json for unit stats and global settings. |
| **ImGui** | Debug UI, Unit counters, parameter tuning. |
| **RVO2** | Reciprocal Velocity Obstacles (ORCA) for unit steering/collision. |
| **stb\_image** | Loading PNG textures (Terrain, Atlas). |
| **Google Test** | Unit and Integration testing framework. |

### **2.2. Directory Structure**

RTS\_Project/  
├── CMakeLists.txt       \# Root build script  
├── Data/                \# Runtime assets  
│   ├── config.json      \# Global & Unit definitions  
│   ├── terrain.png      \# Obstacle map (B\&W)  
│   └── unit\_atlas.png   \# Horizontal sprite strip  
├── src/  
│   ├── main.cpp         \# Entry point, Event Loop  
│   ├── Components.hpp   \# ECS Component definitions  
│   ├── Systems/         \# Logic Implementations  
│   └── Utils/           \# Loaders  
├── tests/               \# Google Test files  
│   ├── CMakeLists.txt  
│   ├── main\_test.cpp  
│   ├── TestSpatialGrid.cpp  
│   └── TestMovementIntegration.cpp

## **3\. Data & Asset Specifications**

### **3.1. Configuration (config.json)**

A single file defining global simulation parameters and specific unit attributes.

* **Global:** Screen resolution, RVO settings (neighbor dist, time horizon), Map tile size, Target Selection Interval.  
* **Factions:** List of colors (RGB) for up to 8 factions.  
* **Atlas Settings:** Definition of sprite size and offsets.

### **3.2. Unit Specifications**

Detailed parameters and behaviors for each unit type. All units are omni-directional (no rotation logic needed for sprites).

#### **Footman (Melee)**

* **Stats:** MaxHP, MovementSpeed, Shield, Range, AttackCooldown, Damage.  
* **Behavior:**  
  * Moves to target.  
  * Attacks single target when within Range.

#### **Archer (Ranged \- Single)**

* **Stats:** MaxHP, MovementSpeed, Shield, Range, FireRate, AttackCooldown, Damage.  
* **Behavior:**  
  * Maintains distance (optional) or moves to Range.  
  * Fires a **Projectile** entity that travels to the target.

#### **Ballista (Ranged \- AOE)**

* **Stats:** MaxHP, MovementSpeed, Shield, Range, FireRate, AttackCooldown, Damage, DamageRange (AOE Radius).  
* **Behavior:**  
  * Fires a **Projectile**.  
  * On impact, deals damage to all enemies within DamageRange of the impact point.

#### **Healer (Support)**

* **Stats:** MaxHP, MovementSpeed, Shield, HealCooldown, HealRange, HealAmount (Negative Damage).  
* **Behavior:**  
  * Heals all units in range HealRange for HealAmount. Every HealCooldown.  
  * Restores HP.

## **4\. ECS Component Design**

Entities are composed of POD (Plain Old Data) structs.

| Component | Data | Purpose |
| :---- | :---- | :---- |
| Position | glm::vec2 | World coordinates. |
| Velocity | glm::vec2 | Current movement vector. |
| Sprite | textureID, uvRect, color | Rendering data. |
| RVOAgent | int agentIndex | ID of the agent in the RVO2 Simulator. |
| UnitType | enum / int | Identifies unit logic (Footman vs Archer). |
| Faction | int | 0-7, determines team and color. |
| TargetPos | glm::vec2 | Destination for movement commands. |
| Stats | HP, MaxHP, Range, Dmg, Shield | Combat statistics. |
| State | Enum (Idle, Move, Attack) | Basic AI state machine. |
| Selection | bool or Tag | Is the unit currently selected? |
| AttackTarget | entt::entity | Entity ID of current enemy target. |

## **5\. Systems Architecture**

### **5.1. Input & Camera System**

* **Camera:** Maintains Zoom and ViewOffset.  
* **Selection:**  
  * **Mouse Drag:** Draws a debug rect using ImGui or GL Lines.  
  * **Mouse Up:** Calculates world bounds AABB. Queries ECS Position components to add Selection tag.  
* **Controls:**  
  * Mouse Wheel: Zoom.  
  * Space \+ Mouse Move: Pan Camera.

### **5.2. Navigation & Command System**

* **Input:** M \+ Click.  
* **Formation Logic:**  
  * Calculates a target rectangle at the click location.  
  * Assigns a unique TargetPos to each selected unit within that rectangle to prevent overlap.  
* **Spatial Grid:**  
  * A Uniform Grid data structure (std::vector of vectors).  
  * Rebuilt or Updated every frame based on Position.  
  * Used for rapid queries: "Find nearest enemy" or "Find units in rect".

### **5.3. Physics System (RVO2 Integration)**

* **Role:** The bridge between EnTT and RVO2 library.  
* **Initialization:** Reads terrain.png and adds static obstacles to RVO::RVOSimulator.  
* **Update Loop:**  
  1. **Sync:** Copy EnTT Position & Velocity \-\> RVO Agent.  
  2. **Pref Velocity:** Calculate (TargetPos \- Pos). Normalize \* Speed. Set as RVO Preferred Velocity.  
  3. **Step:** simulator-\>doStep().  
  4. **Apply:** Copy RVO Agent Position \-\> EnTT Position.

### **5.4. Gameplay Systems**

* **Spawner System:**  
  * S \+ Drag: Calculate density of rectangle. Create entities. Register with RVO.  
  * D \+ Drag: Identify entities in rect. registry.destroy(e). Remove from RVO (teleport to infinity/disable).  
* **Targeting System:**  
  * Runs periodically (every global\_parameter seconds).  
  * Logic:  
    * If current target is dead or nonexistent: Query SpatialGrid for nearest enemy/ally in range.  
    * If current target is alive and in range: Maintain target.  
* **Combat System:**  
  * **Damage Calculation:** FinalDamage \= max(0, IncomingDamage \- Target.Shield).  
  * **Application:** Target.HP \-= FinalDamage.  
  * **Projectiles:** Spawned by Archers/Ballistas. Move to target. On impact, trigger damage logic.  
* **Health System:**  
  * If HP \<= 0, destroy entity.

## **6\. Testing Strategy (Google Test)**

### **6.1. Unit Tests**

* **Spatial Grid:**  
  * TestInsertAndQuery: Insert entities, query a range, verify correct IDs are returned.  
  * TestEmptyGrid: Verify querying an empty grid returns nothing.  
  * TestBoundaryConditions: Insert entities at grid edges.  
* **Math Utils:** Verify vector normalization and distance calculations.  
* **Damage Logic:**  
  * TestShieldReduction: Ensure shield reduces damage correctly.  
  * TestZeroHealth: Ensure system flags unit for death at \<= 0 HP.

### **6.2. Integration Tests**

* **Movement Integration (Headless):**  
  * Spawn a group of units at (0,0).  
  * Issue Move Command to (100,100).  
  * Step simulation for N frames.  
  * **Assert:** All units are within ArrivalRadius of target.  
  * **Assert:** No two units have distance \< UnitRadius \* 2 (Check RVO penetration).  
* **Targeting Integration:**  
  * Spawn 1 Footman and 1 Enemy within range.  
  * Step Targeting System.  
  * **Assert:** Footman has correct AttackTarget component.

## **7\. Control Scheme Summary**

| Action | Input |
| :---- | :---- |
| **Pan Camera** | Space \+ Mouse Move |
| **Zoom Camera** | Mouse Wheel |
| **Select Units** | Left Mouse Drag |
| **Move Command** | M \+ Left Click |
| **Spawn Units** | S \+ Left Mouse Drag |
| **Delete Units** | D \+ Left Mouse Drag |

## **8\. Implementation Plan (Cursor Prompts)**

1. **Project Setup:** CMake, SDL3, EnTT, ImGui, RVO2, **GTest**.  
2. **Resources:** Load Config JSON, Terrain PNG, Atlas PNG.  
3. **ECS Basics:** Render sprites using Atlas & Faction Colors.  
4. **Camera/Input:** Zoom, Pan, Rect Selection.  
5. **Navigation:** Spatial Grid \+ Formation Targets.  
6. **Physics:** Connect EnTT entities to RVO2 Simulator.  
7. **Gameplay:** Combat logic (Shield math), Projectiles, Spawning/Destruction.  
8. **UI:** ImGui overlays for unit counts and FPS.  
9. **Tests:** Implement Unit and Integration tests.

