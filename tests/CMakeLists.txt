
enable_testing()

file(GLOB TEST_SOURCES "*.cpp")

add_executable(RTS_Tests ${TEST_SOURCES})

target_link_libraries(RTS_Tests PRIVATE
    EnTT::EnTT
    GTest::gtest_main
    RVO
)

add_test(NAME AllTests COMMAND RTS_Tests)

