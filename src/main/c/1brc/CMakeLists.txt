cmake_minimum_required(VERSION 4.0)
project(1brc C)

set(CMAKE_C_STANDARD 99)

add_executable(1brc read_bench.c
        fread_bench.c
        iouring_read_bench.c)
include_directories(../../../../local-build/include)
link_libraries(../../../../local-build/lib/liburing.a)
