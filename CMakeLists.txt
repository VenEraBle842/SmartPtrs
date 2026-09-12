cmake_minimum_required(VERSION 3.20)
project(SmartPtrs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(SmartPtrs INTERFACE)
target_include_directories(SmartPtrs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(smart_ptrs_demo
    main.cpp
)
target_link_libraries(smart_ptrs_demo PRIVATE SmartPtrs)
target_compile_options(smart_ptrs_demo PRIVATE -Wall -Wextra)
