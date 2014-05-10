CXX := clang++
LLVMCOMPONENTS := cppbackend
RTTIFLAG := #-fno-rtti
LLVMCONFIG := /usr/bin/llvm-config-3.3

CXXFLAGS := -O0 -g -I$(shell $(LLVMCONFIG) --src-root)/tools/clang/include -I$(shell $(LLVMCONFIG) --obj-root)/tools/clang/include $(shell $(LLVMCONFIG) --cxxflags) $(RTTIFLAG) -Wno-c++11-extensions -std=c++11 -fexceptions
LLVMLDFLAGS := $(shell $(LLVMCONFIG) --ldflags --libs $(LLVMCOMPONENTS))
LINKFLAGS := -ljsoncpp -lboost_filesystem -lboost_system

SOURCES = main.cpp astvisitor.cpp datatypes.cpp astvisitor_visits.cpp hintdb_exporter.cpp processor.cpp hierarcy_tree.cpp hint_db_cache_manager.cpp

OBJECTS = $(SOURCES:.cpp=.o)
EXES = gjumper
#@TODO Optimize CLANGLIBS
CLANGLIBS = \
			-lclangTooling\
			-lclangFrontendTool\
			-lclangFrontend\
			-lclangDriver\
			-lclangSerialization\
			-lclangCodeGen\
			-lclangParse\
			-lclangSema\
			-lclangStaticAnalyzerFrontend\
			-lclangStaticAnalyzerCheckers\
			-lclangStaticAnalyzerCore\
			-lclangAnalysis\
			-lclangARCMigrate\
			-lclangRewriteFrontend\
			-lclangRewriteCore\
			-lclangEdit\
			-lclangAST\
			-lclangLex\
			-lclangBasic\
			$(shell $(LLVMCONFIG) --libs)\
			-lcurses

all: $(OBJECTS) $(EXES)

gjumper: $(OBJECTS)
	$(CXX) -O0 -g -o $@ $(OBJECTS) $(CLANGLIBS) $(LLVMLDFLAGS) $(LINKFLAGS)

clean:
	-rm -f $(EXES) $(OBJECTS) *~
