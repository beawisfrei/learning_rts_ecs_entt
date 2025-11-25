
file(GLOB_RECURSE ALL_SOURCES
    "*.cpp"
    "systems/*.cpp"
    "utils/*.cpp"
    "world/*.cpp"
)

file(GLOB_RECURSE HEADERS
    "*.hpp"
    "systems/*.hpp"
    "utils/*.hpp"
    "components/*.hpp"
)

# Separate main.cpp from library sources
file(GLOB MAIN_SOURCE "main.cpp")
list(REMOVE_ITEM ALL_SOURCES ${MAIN_SOURCE})

# Create static library with all sources except main.cpp
add_library(RTS_Core STATIC ${ALL_SOURCES} ${HEADERS})

target_include_directories(RTS_Core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${stb_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${RVO2_SOURCE_DIR}/src
    ${cereal_SOURCE_DIR}/include
)

# Link dependencies for library
target_link_libraries(RTS_Core PUBLIC
    EnTT::EnTT
    SDL3::SDL3
    nlohmann_json::nlohmann_json
    imgui
    RVO
    opengl32 # Standard Windows OpenGL library
)

# Create executable that links to library
add_executable(RTS_Example ${MAIN_SOURCE})

target_link_libraries(RTS_Example PRIVATE RTS_Core)

# Set working directory to source directory so executables can access original data folder
set_target_properties(RTS_Example PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Copy SDL3.dll to the output directory
if(WIN32)
    add_custom_command(TARGET RTS_Example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:RTS_Example>
    )
endif()
