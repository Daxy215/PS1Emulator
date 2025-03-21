# Add the necessary include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${OPENGL_INCLUDE_DIRS})

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
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imconfig.h
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imgui.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imgui_internal.h
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imstb_rectpack.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imstb_textedit.cpp
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/imstb_truetype.cpp
    
    # GLM sources
    ${CMAKE_SOURCE_DIR}/libs/glm/glm/*.cpp
    
    # Glew sources
    ${CMAKE_SOURCE_DIR}/libs/glew-2.2.0/include/GL/glew.h
    ${CMAKE_SOURCE_DIR}/libs/glew-2.2.0/src/glew.c
)

# Executable definition
add_executable(PS1Emulator ${SOURCES})

# Link OpenGL, glfw and glew
target_link_libraries(PS1Emulator OpenGL glfw ${GLEW_LIBRARIES})

# Add source files of ImGui
target_include_directories(PS1Emulator PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/
    ${CMAKE_SOURCE_DIR}/libs/imgui-1.91.3/backends
)

# Add source files of GLM
target_include_directories(PS1Emulator PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/glm
)

# Add source files of Glew
target_include_directories(PS1Emulator PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/glew-2.2.0/include
)

# Ad source files directories
target_include_directories(PS1Emulator PRIVATE
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

