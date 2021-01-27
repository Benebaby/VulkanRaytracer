cmake_minimum_required(VERSION 3.7)
project(VKR VERSION 1.0.0)


add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
add_compile_definitions(SHADER_PATH="${PROJECT_SOURCE_DIR}/src/shaders" TEXTURE_PATH="${PROJECT_SOURCE_DIR}/src/textures" MODEL_PATH="${PROJECT_SOURCE_DIR}/src/models")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
ENDIF()

file(GLOB SOURCES
    src/*.h
    src/*.cpp
)

add_executable(VKR ${SOURCES})

#add vulkan
find_package(Vulkan)
IF (Vulkan_FOUND)
    target_link_libraries(VKR ${Vulkan_LIBRARIES})
    target_include_directories(VKR PUBLIC ${Vulkan_INCLUDE_DIR})
ELSE()
    message(ERROR "Vulkan SDK has to be installed")
ENDIF()


#add glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glfw)
target_link_libraries(VKR glfw)

#add glm
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glm)
target_link_libraries(VKR glm)

#add glslang
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glslang)
target_link_libraries(VKR glslang SPIRV)

#add imgui, stb, tinyobjloader
include_directories(${PROJECT_SOURCE_DIR}/lib/imgui)
include_directories(${PROJECT_SOURCE_DIR}/lib/stb)
include_directories(${PROJECT_SOURCE_DIR}/lib/tinyobjloader)

