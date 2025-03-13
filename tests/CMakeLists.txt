# Enable testing
enable_testing()

# Define test executable
add_executable(test_com_client main.cc test_receiving.cc test_sending.cc)

# Include com_client
target_include_directories(test_com_client PRIVATE ${CMAKE_SOURCE_DIR}/inc)

# Link against the main library
target_link_libraries(test_com_client PRIVATE com_client)

# Register the tests with CTest
add_test(NAME com_client_tests COMMAND test_com_client)
