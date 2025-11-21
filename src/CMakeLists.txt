
file(GLOB_RECURSE SOURCES
    "*.cpp"
    "systems/*.cpp"
    "utils/*.cpp"
)

file(GLOB_RECURSE HEADERS
    "*.hpp"
    "systems/*.hpp"
    "utils/*.hpp"
    "components/*.hpp"
)

add_executable(RTS_Example ${SOURCES} ${HEADERS})

target_include_directories(RTS_Example PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${stb_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${RVO2_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(RTS_Example PRIVATE
    EnTT::EnTT
    SDL3::SDL3
    nlohmann_json::nlohmann_json
    imgui
    RVO
    opengl32 # Standard Windows OpenGL library
)

# Copy assets to build directory
add_custom_command(TARGET RTS_Example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:RTS_Example>/data
)

# Copy SDL3.dll to the output directory
if(WIN32)
    add_custom_command(TARGET RTS_Example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:RTS_Example>
    )
endif()
