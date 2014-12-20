cmake_minimum_required (VERSION 2.8) 
#@TODO set as param

set(llvm_config_path "/usr/bin/llvm-config-3.3")

add_library(gj_core astvisitor.cpp
                    datatypes.cpp
                    astvisitor_visits.cpp
                    hintdb_exporter.cpp
                    processor.cpp
                    hierarcy_tree.cpp
                    hint_db_cache_manager.cpp
                    fs_utils.cpp
                    communication.cpp)

find_package(Boost COMPONENTS filesystem system) 

set(llvm_components cppbackend)

execute_process(COMMAND ${llvm_config_path} --src-root OUTPUT_VARIABLE llvm_src_root OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${llvm_config_path} --obj-root OUTPUT_VARIABLE llvm_obj_root OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${llvm_config_path} --cxxflags OUTPUT_VARIABLE llvm_cxx_flags OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${llvm_config_path} --libs OUTPUT_VARIABLE llvm_libs OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${llvm_config_path} --libdir OUTPUT_VARIABLE llvm_libdir OUTPUT_STRIP_TRAILING_WHITESPACE)

set(cxxflags -std=c++0x ${llvm_cxx_flags} -Wno-c++11-extensions -fexceptions)
set(includes ${llvm_src_root}/tools/clang/include
                     ${llvm_obj_root}/tools/clang/include
                     ${Boost_INCLUDE_DIRS})

set(ldlibs ${Boost_LIBRARIES}
           jsoncpp
           clangTooling
           clangFrontendTool
           clangFrontend
           clangDriver
           clangSerialization
           clangCodeGen
           clangParse
           clangSema
           clangStaticAnalyzerFrontend
           clangStaticAnalyzerCheckers
           clangStaticAnalyzerCore
           clangAnalysis
           clangARCMigrate
           clangEdit
           clangAST
           clangLex
           clangBasic
           ${llvm_libs}
           curses z pthread ffi rt dl m)

include_directories(${includes})
add_definitions(${cxxflags})

set(core_libdir ${llvm_libdir} PARENT_SCOPE)
set(core_ldlibs ${ldlibs} PARENT_SCOPE)
set(core_includes ${includes} PARENT_SCOPE)
set(core_cxxflags ${cxxflags} PARENT_SCOPE)

