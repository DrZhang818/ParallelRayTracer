cmake_minimum_required(VERSION 3.12)
project(ParallelRayTracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_OPENMP "Enable OpenMP parallel rendering" ON)

add_executable(raytracer
    src/main.cpp
    src/scene.cpp
    src/renderer.cpp
)

target_include_directories(raytracer PRIVATE include)

target_compile_options(raytracer PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/O2 /W4>
)

if(ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(raytracer PRIVATE OpenMP::OpenMP_CXX)
        target_compile_definitions(raytracer PRIVATE PRT_WITH_OPENMP=1)
        message(STATUS "OpenMP enabled")
    else()
        message(WARNING "OpenMP not found; building serial renderer")
    endif()
endif()
