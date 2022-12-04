cmake_minimum_required(VERSION 3.9.4)
project(ProtoVoxel VERSION 0.1.0)

find_package(OpenGL REQUIRED)
find_package(CUDA REQUIRED)

enable_language(CUDA)

add_library(glad ${CMAKE_SOURCE_DIR}/glad/glad.c)
target_include_directories(glad PRIVATE ${CMAKE_SOURCE_DIR})

file(GLOB_RECURSE IMGUI_SRC_FILES ${CMAKE_SOURCE_DIR}/imgui/*.cpp)
add_library(imgui ${IMGUI_SRC_FILES})
target_include_directories(imgui PRIVATE ${CMAKE_SOURCE_DIR}/imgui ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/glfw/include)

file(GLOB_RECURSE SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(ProtoVoxel main.cpp ${SRC_FILES})
set_property(TARGET ProtoVoxel PROPERTY CXX_STANDARD 17)
set_property(TARGET ProtoVoxel PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
set_property(TARGET ProtoVoxel PROPERTY CUDA_SEPARABLE_COMPILATION ON)
set_property(TARGET ProtoVoxel PROPERTY CUDA_ARCHITECTURES 80)

set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/tinygltf)

set_target_properties(ProtoVoxel PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

#add_subdirectory(lua)

target_compile_options(ProtoVoxel PRIVATE -mavx2 -mbmi)
target_link_directories(ProtoVoxel PRIVATE ${CMAKE_SOURCE_DIR}/glfw/lib-vc2019)
target_link_libraries(ProtoVoxel glfw glad imgui OpenGL::GL ${CMAKE_DL_LIBS}) #lua_static
target_compile_definitions(ProtoVoxel PRIVATE DEBUG)
target_compile_definitions(ProtoVoxel PRIVATE _USE_MATH_DEFINES)
target_include_directories(ProtoVoxel PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/glfw/include ${CMAKE_SOURCE_DIR}/tinygltf)# ${CMAKE_SOURCE_DIR}/Lua/lua-5.4.3/include)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)