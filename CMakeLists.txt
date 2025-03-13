cmake_minimum_required(VERSION 3.10)
project(com_client)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

# Gather source and header files
file(GLOB_RECURSE HEADERS "inc/*.h")
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create the library
add_library(com_client ${SOURCES} ${HEADERS})

# Link dependencies
target_link_libraries(com_client PUBLIC quill::quill)

# Ensure headers are accessible
target_include_directories(com_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# Only add tests if this is the main project (not included in another project)
if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
