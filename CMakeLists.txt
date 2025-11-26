cmake_minimum_required(VERSION 3.14)

project(RTS_ECS_Example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Compiler-specific optimization flags
if(MSVC)
	# MSVC Release optimizations
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL /Oi")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
	set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
	set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
	
	# MSVC Debug flags (usually already set by default, but explicit here)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
else()
	# GCC/Clang Release optimizations
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -DNDEBUG")
	
	# GCC/Clang Debug flags
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
endif()

include(FetchContent)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# --- EnTT ---
FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.13.0
)
FetchContent_MakeAvailable(entt)

# --- SDL3 ---
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG main  # Using main as requested for latest
)
FetchContent_MakeAvailable(SDL3)

# --- Glad (REMOVED) ---
# Glad is removed to avoid python dependency issues. 
# We are using a custom gl_loader in src/utils.

# --- nlohmann_json ---
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# --- ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# Add imgui library target manually if not defined (common issue with bare ImGui repos)
if(NOT TARGET imgui)
    add_library(imgui STATIC 
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
    
    # ImGui backends need access to SDL headers
    # Since we're compiling backend implementation files inside the imgui target,
    # the imgui target needs to link to SDL3 to find its headers.
    target_link_libraries(imgui PUBLIC SDL3::SDL3)
endif()

# --- RVO2 ---
FetchContent_Declare(
    RVO2
    GIT_REPOSITORY https://github.com/snape/RVO2.git
    GIT_TAG main
)
FetchContent_MakeAvailable(RVO2)

# --- stb ---
# stb is a header-only library, usually just downloaded.
# We can use a wrapper or just download the file.
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# --- Cereal ---
# Disable tests and examples to avoid Boost dependency
set(SKIP_PORTABILITY_TEST ON CACHE BOOL "" FORCE)
set(SKIP_PERFORMANCE_COMPARISON ON CACHE BOOL "" FORCE)
set(BUILD_DOC OFF CACHE BOOL "" FORCE)
set(BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
set(WITH_WERROR OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    cereal
    GIT_REPOSITORY https://github.com/USCiLab/cereal.git
    GIT_TAG v1.3.2
)
FetchContent_MakeAvailable(cereal)

# --- GoogleTest ---
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- Tracy Profiler ---
set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
FetchContent_Declare(
	tracy
	GIT_REPOSITORY https://github.com/wolfpld/tracy.git
	GIT_TAG master
)
FetchContent_MakeAvailable(tracy)

add_subdirectory(src)
add_subdirectory(tests)
