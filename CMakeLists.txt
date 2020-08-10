project(Excal)
cmake_minimum_required(VERSION 3.10)

set(lib_path /usr/local/lib/)
set(CMAKE_BUILD_TYPE Debug)

# Debug build
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fsanitize=address -std=c++17" )
# Optimized build
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++17" )

file(GLOB SOURCE_FILES "src/*.cpp" "app/*.cpp")
file(GLOB HEADER_FILES "src/*.h" "app/*.h")

include_directories("src" "app")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)

if (VULKAN_FOUND)
  message(STATUS "Found Vulkan, Including and Linking now")
  include_directories(
    ${Vulkan_INCLUDE_DIRS}
    "${lib_path}stb"
    "${lib_path}tinyobjloader"
    "${lib_path}vulkan-memory-allocator"
  )
	target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw)
endif (VULKAN_FOUND)

# GLSLC command to compile shaders to SPIR-V
find_program(GLSLC glslc)
set(shader_path ${CMAKE_HOME_DIRECTORY}/shaders/)
execute_process(COMMAND "rm ${shader_path}*.spv")
file(GLOB shaders RELATIVE ${CMAKE_SOURCE_DIR} "${shader_path}*.vert" "${shader_path}*.frag")
foreach(shader ${shaders})
  set(input_glsl "${CMAKE_HOME_DIRECTORY}/${shader}")
  set(output_spv "${input_glsl}.spv")
  execute_process(COMMAND "glslc" "${input_glsl}" "-o" "${output_spv}")
endforeach()
