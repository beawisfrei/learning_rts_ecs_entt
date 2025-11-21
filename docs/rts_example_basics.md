We are making RTS example because I want to try EnTT as ECS.

High-level decisions:

* Application  
  * Should be cross-platform: Windows & MacOS  
  * Should use SDL3 for input and window  
  * Should use Open GL for rendering  
  * Should use EnTT for ECS  
* Project  
  * Should use CMake for project generation and dependencies  
  * Should target C++17  
  * Would be compiled on Microsoft Visual Studio Community Edition  
  * All UI should be done via ImGUI  
* Input files  
  * All data files will be stored in folder Data  
  * All parameters for the code are in one JSON file. It contains both global and unit parameters. For the sake of simplicity.  
  * Terrain is stored in PNG file. This file contains black and white pixels. White for obstacle tiles, black for flat terrain. Size of map is defined by resolution of texture and tile size in parameters JSON.  
* Units  
  * There are several types of units  
  * They have the same size (defined in parameters)  
  * Units are omni-directional \- so they don’t rotate  
  * Unit has faction. There would be 8 max factions.  
* Rendering should be done via OpenGL  
  * Terrain texture is rendered as is with texture pixel multiplied by color specified in parameters.  
  * Units are rendered as sprites.   
    * Color defined by faction (all faction colors are in parameters)  
    * Selected unit has color defined in parameters. White by default.  
    * Sprite texture is defined by unit type.  
      * In parameters there should be one Unit Texture atlas.  
      * It will be horizontal atlas.   
      * Unit parameters will specify U coordinate offset in this texture. Global parameters would say how much unit textures are stored in this atlas.  
* Camera controls  
  * Zoom (Mouse Wheel)  
  * Pan (Mouse Move \+ Space)  
* Units Controls  
  * Rectangular selection (Start via Mouse Down, Mouse Move, End with Mouse Up) \- select units  
    * Amount of units for each unit type is shown in UI  
  * Key M \+ Click \= Move  
  * Key S \+ Rectangular selection \= Spawn units in this area in rectangular formation (amount of units, type and faction are specified in UI)  
  * Key D \+ Rectangular selection \= Remove all units in this area  
* Navigation  
  * When we send units by move command we use rectangular formation and calculate each units offset from the click position  
  * We would Uniform Spatial Grid to find nearest units for targeting and ORCA  
  * We would use ORCA for steering  
  * For the simplicity we would not use A\* for navigation, lets assume steering would be enough and all obstacles would be convex

Unit Types

* Footman (Melee)  
  * MaxHP  
  * MovementSpeed  
  * Shield  
  * Range  
  * AttackColdown  
  * Damage  
* Archer (Shoots projectiles that target one enemy unit)  
  * MaxHP  
  * MovementSpeed  
  * Shield  
  * Range  
  * FireRate  
  * AttackColdown  
  * Damage  
* Ballista (Shoots projectiles that deal AOE damage)  
  * MaxHP  
  * MovementSpeed  
  * Shield  
  * Range  
  * FireRate  
  * AttackColdown  
  * DamageRange  
  * Damage  
* Healer  
  * MaxHP  
  * MovementSpeed  
  * Shield  
  * HealColdown  
  * HealRange

Damage Dealing System

* HP \-= (Damage of unit or projectile \- Shield value)

Unit logic

* If has no attack target and move target  
  * Stay  
* If has no attack target  
  * Move to a designated place  
* Attack or wait while attack cooldown finishes  
  * If current target is dead \- do nothing

Every \<global\_parameter\> seconds we run target selection for all units regardless they do have target or not

* Each unit can have its own selection logic. But we’ll start with default one:   
  * If current target is alive and is in range \- is stays current target  
  * If the current target is dead or there is no target \- we select any enemy unit within attack range.

