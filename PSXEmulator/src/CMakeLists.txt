# Add the necessary include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${OPENGL_INCLUDE_DIRS})

set(LIBS_DIR "${CMAKE_SOURCE_DIR}/../libs")

# Check if it exists
if (EXISTS "${LIBS_DIR}")
    message(STATUS "Found libs folder at: ${LIBS_DIR}")
else()
    message(FATAL_ERROR "'libs' folder not found at: ${LIBS_DIR}\n"
            "Expected structure:\n"
            "  ${CMAKE_SOURCE_DIR}/../libs/\n"
            "Make sure your project root contains 'libs/'.")
endif()

# Files to be included
file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/src/Core/*.cpp
    ${CMAKE_SOURCE_DIR}/src/CPU/*.cpp
    ${CMAKE_SOURCE_DIR}/src/CPU/COP/*.cpp
    ${CMAKE_SOURCE_DIR}/src/CPU/COP/Stolen/gte/*.cpp
    ${CMAKE_SOURCE_DIR}/src/DMA/*.cpp
    ${CMAKE_SOURCE_DIR}/src/GPU/*.cpp
    ${CMAKE_SOURCE_DIR}/src/GPU/Rendering/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/Bios/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/CDROM/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/IO/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/IO/Peripherals/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/IO/Memories/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Memory/Timers/*.cpp
    ${CMAKE_SOURCE_DIR}/src/SPU/*.cpp
    ${CMAKE_SOURCE_DIR}/src/SPU/Stolen/*.cpp
    ${CMAKE_SOURCE_DIR}/src/SPU/Utils/*.cpp
    ${CMAKE_SOURCE_DIR}/src/SPU/Utils/FileSystem/*.cpp
    
    # ImGui sources
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/imconfig.h
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/imgui.cpp
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/imgui_internal.h
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/imstb_rectpack.cpp
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/imstb_textedit.cpp
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/imstb_truetype.cpp 
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/backends/imgui_impl_opengl3.cpp
    
    # GLM sources
    # ${CMAKE_SOURCE_DIR}/../libs/glm/glm/*.cpp
    
    # Glew sources
    ${CMAKE_SOURCE_DIR}/../libs/glew-2.2.0/include/GL/glew.h
    ${CMAKE_SOURCE_DIR}/../libs/glew-2.2.0/src/glew.c
)

set(NLOHMANN_PATH ${LIBS_DIR}/nlohmann)
add_library(nlohmann INTERFACE)
target_include_directories(nlohmann INTERFACE ${NLOHMANN_PATH})

# Executable definition
add_executable(PS1Emulator ${SOURCES}
        Memory/MDEC/MDEC.cpp
        Memory/MDEC/MDEC.h)

# Link OpenGL, glfw, and glew
find_package(OpenGL REQUIRED)

target_link_libraries(PS1Emulator
        ${OPENGL_LIBRARIES}
        glfw
        GLEW
        X11
        dl
        pthread
        nlohmann
)

# Add include directories for ImGui, GLM, Glew, and other sources
target_include_directories(PS1Emulator PRIVATE
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/
    ${CMAKE_SOURCE_DIR}/../libs/imgui-1.91.9b/backends
    ${CMAKE_SOURCE_DIR}/../libs/glm
    ${CMAKE_SOURCE_DIR}/../libs/glew-2.2.0/include
    ${CMAKE_SOURCE_DIR}/src/Core/
    ${CMAKE_SOURCE_DIR}/src/CPU
    ${CMAKE_SOURCE_DIR}/src/CPU/COP
    ${CMAKE_SOURCE_DIR}/src/CPU/COP/Stolen/gte
    ${CMAKE_SOURCE_DIR}/src/DMA
    ${CMAKE_SOURCE_DIR}/src/GPU
    ${CMAKE_SOURCE_DIR}/src/GPU/Rendering
    ${CMAKE_SOURCE_DIR}/src/Memory
    ${CMAKE_SOURCE_DIR}/src/Memory/Bios
    ${CMAKE_SOURCE_DIR}/src/Memory/CDROM
    ${CMAKE_SOURCE_DIR}/src/Memory/IO
    ${CMAKE_SOURCE_DIR}/src/Memory/IO/Peripherals
    ${CMAKE_SOURCE_DIR}/src/Memory/IO/Memories
    ${CMAKE_SOURCE_DIR}/src/Memory/Timers
    ${CMAKE_SOURCE_DIR}/src/SPU
    ${CMAKE_SOURCE_DIR}/src/SPU/Stolen
    ${CMAKE_SOURCE_DIR}/src/SPU/Utils
    ${CMAKE_SOURCE_DIR}/src/SPU/Utils/FileSystem
)
