cmake_minimum_required(VERSION 3.15)
project(hlsl_to_spirv_dxc)

# C++17 requires
set(CMAKE_CXX_STANDARD 17)

# Location of DXC
set(DXC_ROOT "${PROJECT_SOURCE_DIR}/3rd")

# Adding executable
add_executable(hlsl_to_spirv_dxc hlsl_to_spirv_dxc.cpp)

# Compilation macro
target_compile_definitions(hlsl_to_spirv_dxc PRIVATE SHADER_SOURCE_DIR="${CMAKE_SOURCE_DIR}/shaders")

# Include dir
target_include_directories(hlsl_to_spirv_dxc PRIVATE ${DXC_ROOT}/include)

# Libraries
if(WIN32)
target_link_libraries(hlsl_to_spirv_dxc PRIVATE ${DXC_ROOT}/lib/dxcompiler)
else()
target_link_libraries(hlsl_to_spirv_dxc PRIVATE ${DXC_ROOT}/lib/libdxcompiler.so)
endif()