cmake_minimum_required(VERSION 3.22)
project(vulkan_terrain LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/bin)
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found. Did you source the Vulkan SDK setup script?")
endif()
find_path(GLM_INCLUDE_DIR glm/glm.hpp
    HINTS
        /opt/homebrew/include
        /usr/local/include
)
if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "GLM not found. Try: brew install glm")
endif()
set(SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BIN ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_BIN})
set(SHADERS
    terrain.comp
    terrain.vert
    terrain.frag
    foliage.vert
    foliage.frag
    sky.vert
    sky.frag
    ui.vert
    ui.frag
    water.vert
    water.frag
)
set(SPV_OUTPUTS "")
foreach(SHADER ${SHADERS})
    add_custom_command(
        OUTPUT ${SHADER_BIN}/${SHADER}.spv
        COMMAND ${GLSLC} ${SHADER_SRC}/${SHADER} -o ${SHADER_BIN}/${SHADER}.spv
        DEPENDS ${SHADER_SRC}/${SHADER}
        COMMENT "Compiling ${SHADER}"
    )
    list(APPEND SPV_OUTPUTS ${SHADER_BIN}/${SHADER}.spv)
endforeach()
add_custom_target(shaders ALL DEPENDS ${SPV_OUTPUTS})
file(GLOB_RECURSE SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(terrain ${SOURCES})
add_dependencies(terrain shaders)
target_include_directories(terrain PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GLM_INCLUDE_DIR}
)
target_link_libraries(terrain PRIVATE
    Vulkan::Vulkan
    glfw
)
