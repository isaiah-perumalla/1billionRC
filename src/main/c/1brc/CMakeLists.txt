cmake_minimum_required(VERSION 4.0)
project(1brc C)

set(CMAKE_C_STANDARD 99)

add_executable(1brc read_bench.c
        fread_bench.c)
