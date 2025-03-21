# Minumum requirement
cmake_minimum_required(VERSION 3.10)

# Define the name of the project
project(PS1Emulator)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2 package
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

find_package(GLEW REQUIRED)

# Include GLEW in your project
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    message(STATUS "GLEW found: ${GLEW_VERSION}")
else()
    message(FATAL_ERROR "GLEW not found!")
endif()

# Copy the "Roms" folder to the built path
file(COPY ${CMAKE_SOURCE_DIR}/BIOS       DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/ROMS       DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/MemoryCard DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/Shaders    DESTINATION ${CMAKE_BINARY_DIR})

# Enable debugging symbols for Debug builds
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# Include the "src" file
add_subdirectory(src)
