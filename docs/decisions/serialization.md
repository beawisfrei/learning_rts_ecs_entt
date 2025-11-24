Serialization in C++ is generally difficult because the language lacks built-in reflection. In EnTT, the challenge is doubled: you aren't just saving data; you are saving **relationships** (Entity A refers to Entity B).

The standard, industry-accepted "best approach" for EnTT is to pair it with **Cereal**.

**Cereal** is a header-only serialization library that supports JSON, XML, and Binary. EnTT has built-in support to pipe its registry directly into Cereal archives.

Here is the step-by-step implementation for a robust JSON Save/Load system.

### 1\. The Setup (Dependencies)

You need `cereal` included in your project.

```cpp
#include <cereal/archives/json.hpp>
#include <entt/entt.hpp>
```

### 2\. Prepare Your Components

Cereal needs to know *how* to save your specific structs. You add a `serialize` template function to your components.

```cpp
struct Position {
    float x, y;

    // This function tells Cereal what fields to save
    template<class Archive>
    void serialize(Archive &archive) {
        archive(CEREAL_NVP(x), CEREAL_NVP(y)); // NVP = Name Value Pair (for JSON keys)
    }
};

struct Health {
    int current, max;

    template<class Archive>
    void serialize(Archive &archive) {
        archive(CEREAL_NVP(current), CEREAL_NVP(max));
    }
};
```

### 3\. Saving (The Snapshot)

EnTT provides `entt::snapshot`. This utility creates a view over the registry and "pumps" data into the archive.

```cpp
#include <fstream>

void SaveGame(entt::registry &reg) {
    std::ofstream os("savegame.json");
    cereal::JSONOutputArchive archive(os);

    // 1. Create a snapshot of the registry
    entt::snapshot snapshot{reg};

    // 2. Serialize all entities (the IDs themselves)
    snapshot.entities(archive) 
        
    // 3. Serialize specific components
    // You must list EVERY component you want to save here
        .component<Position>(archive)
        .component<Health>(archive)
        .component<Movement>(archive);
}
```

### 4\. Loading (The Loader)

Loading is trickier because you are creating entities. EnTT provides `entt::snapshot_loader`.

```cpp
void LoadGame(entt::registry &reg) {
    // Clear current state first!
    reg.clear(); 

    std::ifstream is("savegame.json");
    cereal::JSONInputArchive archive(is);

    entt::snapshot_loader loader{reg};

    loader.entities(archive)
        .component<Position>(archive)
        .component<Health>(archive)
        .component<Movement>(archive);
        
    // IMPORTANT: The orphaned function cleans up entities 
    // that might have been created by the loader but have no components.
    loader.orphans(); 
}
```

### The "Hard Part": Entity References

If your `AttackTarget` component stores an `entt::entity`, you have a problem.
When you load the game, EnTT might assign **new** integer IDs to the entities. If `Unit A` was ID `5` and `Unit B` targeted `5`, but on load `Unit A` becomes ID `10`, the target link is broken.

EnTT handles this automatically **IF** you instruct it to map the relationships.

#### 1\. Update the Component

```cpp
struct AttackTarget {
    entt::entity target;

    template<class Archive>
    void serialize(Archive &archive) {
        // Just save the integer for now
        archive(CEREAL_NVP(target)); 
    }
};
```

#### 2\. Use the Continuous Loader

If you are loading into an empty registry, the standard loader (above) usually preserves IDs. However, if you want to be safe (or merge into an existing world), use `continuous_loader`.

But the most robust way to handle references in Cereal + EnTT is to **map** them during the component load phase.

```cpp
// Advanced Loading with Reference Fix-up
void LoadGameWithRefs(entt::registry &reg) {
    reg.clear();
    std::ifstream is("savegame.json");
    cereal::JSONInputArchive archive(is);

    // This map will store: Old_ID (from JSON) -> New_ID (in current Registry)
    std::map<entt::entity, entt::entity> map;

    entt::continuous_loader loader{reg};

    // 1. Load Entities and populate the map
    loader.entities(archive)
          .map<entt::entity>(map); // <--- Tracks ID changes

    // 2. Load Components
    loader.component<Position>(archive)
          .component<Health>(archive);

    // 3. Load Relational Components (Manual fix-up)
    // Ideally, you write a custom helper, but here is the logic:
    loader.component<AttackTarget>(archive);
    
    // POST-PROCESS: Fix the references
    auto view = reg.view<AttackTarget>();
    for(auto entity : view) {
        auto &at = view.get<AttackTarget>(entity);
        if (at.target != entt::null) {
            // Update the target using the map we built
            if(map.contains(at.target)) {
                 at.target = map[at.target];
            } else {
                 at.target = entt::null; // Reference died
            }
        }
    }
}
```

### Summary of Best Practices

1.  **Use Cereal:** Don't write your own JSON parser. Cereal handles types, vectors, and maps seamlessly.
2.  **Explicit List:** You must manually list every component type in the `.component<T>()` chain. If you forget one, it won't save.
3.  **Tags for Consistency:** If you need to find a specific unit (like "The Player") across saves, don't rely on the Entity ID (integer). Attach a unique identifier component (e.g., a UUID string or a special Tag component) and search for that after loading.

### Next Step

This setup works for saving the *entire* state. Would you like to know how to create **Prefabs** (saving a single entity to JSON and spawning copies of it later)?