
enable_testing()

file(GLOB TEST_SOURCES "*.cpp")

add_executable(RTS_Tests ${TEST_SOURCES})

target_include_directories(RTS_Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(RTS_Tests PRIVATE
    RTS_Core
    GTest::gtest_main
    nlohmann_json::nlohmann_json
)

# Copy SDL3.dll to the test output directory
if(WIN32)
	add_custom_command(TARGET RTS_Tests POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:SDL3::SDL3>
		$<TARGET_FILE_DIR:RTS_Tests>
	)
endif()

add_test(NAME AllTests COMMAND RTS_Tests) 
set_tests_properties(AllTests PROPERTIES
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

