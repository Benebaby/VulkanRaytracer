cmake_minimum_required(VERSION 3.7)
project(Test VERSION 1.0.0)
add_executable(Test src/main.cpp)


add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
add_compile_definitions(SHADER_PATH="${PROJECT_SOURCE_DIR}/src/shaders")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
ENDIF()


#add vulkan
find_package(Vulkan)
IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ENDIF()
target_link_libraries(Test ${Vulkan_LIBRARIES})
target_include_directories(Test PUBLIC ${Vulkan_INCLUDE_DIR})


#add glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glfw-3.3.2)
target_link_libraries(Test glfw)

#add glm
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glm-0.9.9.9)
target_link_libraries(Test glm)