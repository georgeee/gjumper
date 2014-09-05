cmake_minimum_required (VERSION 2.8) 

add_subdirectory(core)
add_definitions(${core_cxxflags})
include_directories(core ${core_includes})


add_executable(gjumper_client gjumper_client.cpp)
target_link_libraries(gjumper_client gj_core ${core_ldlibs})
set_target_properties(gjumper_client PROPERTIES LINK_FLAGS "-L${core_libdir}")

add_executable(gjcc gjcc_utilite.cpp)
target_link_libraries(gjcc gj_core ${core_ldlibs})
set_target_properties(gjcc PROPERTIES LINK_FLAGS "-L${core_libdir}")
set_target_properties(gjcc PROPERTIES COMPILE_FLAGS "-DGJUMPER_GJCC_C")

add_executable(gjccxx gjcc_utilite.cpp)
target_link_libraries(gjccxx gj_core ${core_ldlibs})
set_target_properties(gjccxx PROPERTIES LINK_FLAGS "-L${core_libdir}")
set_target_properties(gjccxx PROPERTIES COMPILE_FLAGS "-DGJUMPER_GJCC_CXX")

install(TARGETS gjumper_client gjcc gjccxx DESTINATION .)
