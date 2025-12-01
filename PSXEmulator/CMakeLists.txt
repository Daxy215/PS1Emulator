# Minumum requirement
cmake_minimum_required(VERSION 3.10)

# Define the name of the project
project(PS1Emulator)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0072 NEW)  # Prefer GLVND over legacy
set(OpenGL_GL_PREFERENCE GLVND)

# Find SDL2 package
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

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
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -g -O0")
set(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -g -O0")
set(CMAKE_STRIP FALSE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer")

add_subdirectory(src)
