cmake_minimum_required (VERSION 2.8) 

project (GJumper)

add_subdirectory(src)

install(PROGRAMS gj_compile.sh DESTINATION .)
install(FILES gj.vim DESTINATION .)
