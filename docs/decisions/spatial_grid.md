You are hitting on the exact bottleneck of spatial partitioning in ECS: **Memory Allocation**.

If you use `std::vector<std::entity>` inside every cell of your grid, you will kill performance with cache misses and reallocations when 100k units start moving across cell boundaries.

The "Gold Standard" high-performance approach for ECS spatial grids is an **Intrusive Doubly-Linked List**.

### The Concept: The Entity *is* the Node

Instead of the grid storing a list of entities, the grid stores the **Head** entity ID. The entities themselves store the `Next` and `Prev` entity IDs as a component.

**Why is this Allocation Free?**

1.  **Grid Storage:** `std::vector<entt::entity> buckets`. Fixed size at startup. Zero allocation at runtime.
2.  **Entity Storage:** You add a `SpatialNode` component to your units. This is just ECS data.
3.  **Movement:** Moving from Cell A to Cell B is just integer swapping (changing the `next` and `prev` IDs). No `malloc`, no `free`, no `std::vector::resize`.

Here is the robust implementation:

### 1\. The Component

This component makes the entity part of the grid.

```cpp
struct SpatialNode {
    // Intrusive Linked List connections
    entt::entity next = entt::null;
    entt::entity prev = entt::null;
    
    // Cache the cell index so we know where to remove it from later
    int cell_index = -1; 
};
```

### 2\. The Grid Class

I have replaced your `std::vector` return types with a **Template Callback**. This is much faster than filling a vector because it allows the compiler to inline your logic directly into the query loop.

```cpp
class SpatialGrid {
private:
    int _width, _height;
    int _cell_size;
    int _cols, _rows;

    // The grid only stores the "Head" of the list for that cell
    std::vector<entt::entity> _cells;
    entt::registry& _reg;

    int get_cell_index(const Vec2& pos) const {
        int x = static_cast<int>(pos.x) / _cell_size;
        int y = static_cast<int>(pos.y) / _cell_size;
        // Clamp to boundaries to prevent crash
        x = std::max(0, std::min(x, _cols - 1));
        y = std::max(0, std::min(y, _rows - 1));
        return x + y * _cols;
    }

public:
    SpatialGrid(entt::registry& registry, int width, int height, int cell_size) 
        : _reg(registry), _width(width), _height(height), _cell_size(cell_size) {
        _cols = width / cell_size;
        _rows = height / cell_size;
        _cells.resize(_cols * _rows, entt::null);
    }

    // O(1) - No Allocations
    void insert(entt::entity entity, const Vec2& pos) {
        int index = get_cell_index(pos);
        auto& node = _reg.get_or_emplace<SpatialNode>(entity);

        node.cell_index = index;
        node.next = _cells[index]; // Old head becomes next
        node.prev = entt::null;    // We are the new head, no prev

        // If there was an old head, update its prev to point to us
        if (node.next != entt::null) {
            _reg.get<SpatialNode>(node.next).prev = entity;
        }

        // We become the new head
        _cells[index] = entity;
    }

    // O(1) - No Allocations
    void remove(entt::entity entity) {
        if (!_reg.all_of<SpatialNode>(entity)) return;

        auto& node = _reg.get<SpatialNode>(entity);
        int index = node.cell_index;

        if (index == -1) return; // Not in grid

        // 1. Unlink Prev
        if (node.prev != entt::null) {
            _reg.get<SpatialNode>(node.prev).next = node.next;
        } else {
            // If no prev, we were the head. Update the bucket.
            _cells[index] = node.next;
        }

        // 2. Unlink Next
        if (node.next != entt::null) {
            _reg.get<SpatialNode>(node.next).prev = node.prev;
        }

        node.cell_index = -1;
    }

    // O(1) - The "Movement System" calls this
    void update(entt::entity entity, const Vec2& old_pos, const Vec2& new_pos) {
        int old_idx = get_cell_index(old_pos);
        int new_idx = get_cell_index(new_pos);

        // Only overhead is if we actually crossed a cell boundary
        if (old_idx != new_idx) {
            remove(entity);
            insert(entity, new_pos);
        }
    }

    // --- QUERIES ---

    // Zero-Allocation Query using Lambda
    template<typename Func>
    void query_rect(const Vec2& min, const Vec2& max, Func&& callback) {
        int start_x = static_cast<int>(min.x) / _cell_size;
        int end_x = static_cast<int>(max.x) / _cell_size;
        int start_y = static_cast<int>(min.y) / _cell_size;
        int end_y = static_cast<int>(max.y) / _cell_size;

        // Clamp
        start_x = std::max(0, start_x); end_x = std::min(_cols - 1, end_x);
        start_y = std::max(0, start_y); end_y = std::min(_rows - 1, end_y);

        for (int y = start_y; y <= end_y; ++y) {
            for (int x = start_x; x <= end_x; ++x) {
                entt::entity curr = _cells[x + y * _cols];

                // Traverse the linked list in this cell
                while (curr != entt::null) {
                    const auto& node = _reg.get<SpatialNode>(curr);
                    
                    // Execute the callback (Filtering happens in the user lambda)
                    callback(curr);

                    curr = node.next;
                }
            }
        }
    }
    
    // Implementation of find_nearest returning just one entity
    entt::entity find_nearest(const Vec2& pos, float radius, int faction = -1) {
        entt::entity best_entity = entt::null;
        float best_dist_sq = radius * radius;

        Vec2 min = {pos.x - radius, pos.y - radius};
        Vec2 max = {pos.x + radius, pos.y + radius};

        query_rect(min, max, [&](entt::entity e) {
            // Manual distance check
            const auto& target_pos = _reg.get<Vec2>(e); // Assuming Vec2 component
            
            // Faction filter (if your game uses a Faction component)
            if(faction != -1) {
                 if(_reg.get<Faction>(e).id != faction) return;
            }

            float dx = target_pos.x - pos.x;
            float dy = target_pos.y - pos.y;
            float dist_sq = dx*dx + dy*dy;

            if (dist_sq < best_dist_sq) {
                best_dist_sq = dist_sq;
                best_entity = e;
            }
        });

        return best_entity;
    }
};
```

### 3\. Usage in Systems

#### The Movement System Integration

This is where the magic happens. You don't update the grid every frame. You update it *only* when the entity moves enough to matter.

```cpp
void MovementSystem(entt::registry& reg, SpatialGrid& grid, float dt) {
    auto view = reg.view<Position, Velocity, SpatialNode>();

    for(auto [entity, pos, vel, node] : view.each()) {
        Vec2 old_pos = pos;
        
        // Physics Logic
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;

        // Update Grid (Only does work if cell index changes)
        // We pass old_pos specifically so we don't have to calculate cell index 
        // from scratch if we don't want to, but the update function handles it.
        grid.update(entity, old_pos, pos);
    }
}
```

#### The Query Usage (No Vectors\!)

Instead of getting a vector back and iterating it (Double Iteration), you inject your logic directly.

```cpp
void ExplosionSystem(entt::registry& reg, SpatialGrid& grid, Vec2 bomb_pos) {
    float radius = 50.0f;
    Vec2 min = {bomb_pos.x - radius, bomb_pos.y - radius};
    Vec2 max = {bomb_pos.x + radius, bomb_pos.y + radius};

    // "Allocation Free" query
    grid.query_rect(min, max, [&](entt::entity target) {
        // This runs IMMEDIATE for every candidate
        auto& target_pos = reg.get<Position>(target);
        if (Distance(bomb_pos, target_pos) <= radius) {
            reg.emplace<Damage>(target, 100);
        }
    });
}
```

### Performance Analysis vs `std::vector` Grid

| Feature | `std::vector` Cells | Intrusive Linked List (Recommended) |
| :--- | :--- | :--- |
| **Grid Memory** | Huge (Vector headers per cell) | Tiny (1 integer per cell) |
| **Insert** | May allocate (vector resize) | **Zero Allocation** |
| **Remove** | `O(N)` (Must search vector to erase) | **O(1)** (Direct pointer unlink) |
| **Move** | Slow (Erase + Push) | **Fast** (Pointer swap) |
| **Cache** | Bad (Vector data is scattered) | **Good** (Component data is packed) |

**One final optimization tip:**
Since `SpatialNode` is accessed extremely frequently (every time you traverse the list), ensure `SpatialNode` is in a separate component pool from your "heavy" data. EnTT does this automatically, so you are good to go. This ensures that when the CPU prefetches `node.next`, it pulls in the next few nodes into the cache line automatically.