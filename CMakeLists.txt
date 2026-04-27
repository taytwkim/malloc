cmake_minimum_required(VERSION 3.16)

project(taymalloc VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(taymalloc SHARED
    src/arena.c
    src/config.c
    src/freelist.c
    src/heap.c
    src/malloc.c
    src/platform.c
)

target_include_directories(taymalloc
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_compile_options(taymalloc
    PRIVATE
        -Wall
        -Wextra
        -O2
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(taymalloc PRIVATE _GNU_SOURCE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(taymalloc PROPERTIES SUFFIX ".dylib")
else()
    message(FATAL_ERROR "taymalloc currently supports Linux and macOS only.")
endif()

find_package(Threads REQUIRED)
target_link_libraries(taymalloc PRIVATE Threads::Threads)

add_executable(taymalloc_hello tests/hello.c)
target_link_libraries(taymalloc_hello PRIVATE Threads::Threads)
set_target_properties(taymalloc_hello PROPERTIES OUTPUT_NAME hello)

add_executable(taymalloc_sequential tests/sequential.c)
target_link_libraries(taymalloc_sequential PRIVATE Threads::Threads)
set_target_properties(taymalloc_sequential PROPERTIES OUTPUT_NAME sequential)

find_package(OpenMP)
if(OpenMP_C_FOUND)
    add_executable(taymalloc_parallel tests/parallel.c)
    target_link_libraries(taymalloc_parallel PRIVATE Threads::Threads OpenMP::OpenMP_C)
    set_target_properties(taymalloc_parallel PROPERTIES OUTPUT_NAME parallel)
endif()
