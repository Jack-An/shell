cmake_minimum_required(VERSION 3.10)
project(MyShell C)

set(CMAKE_C_STANDARD 11)

add_executable(MyShell main.c shell.h)
target_link_libraries(MyShell -lreadline -lncurses)