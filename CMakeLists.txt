cmake_minimum_required (VERSION 2.8) 

project (GJumper)

add_subdirectory(src)

install(PROGRAMS gjumper.sh DESTINATION .)
install(FILES gj.vim DESTINATION .)
