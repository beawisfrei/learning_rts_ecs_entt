#pragma once

#include <tracy/Tracy.hpp>

// Tracy profiling macros
// Use ZoneScoped for automatic scoped zones
// Use ZoneScopedN for named zones
// Use FrameMark to mark frame boundaries

constexpr int TRACY_COLOR_UPDATE = 0x0000FF;