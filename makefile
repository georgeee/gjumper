CXX := clang++
LLVMCOMPONENTS := cppbackend
RTTIFLAG := #-fno-rtti
LLVMCONFIG := /usr/bin/llvm-config-3.3

CXXFLAGS := -g -I$(shell $(LLVMCONFIG) --src-root)/tools/clang/include -I$(shell $(LLVMCONFIG) --obj-root)/tools/clang/include $(shell $(LLVMCONFIG) --cxxflags) $(RTTIFLAG) -Wno-c++11-extensions -std=c++11
LLVMLDFLAGS := $(shell $(LLVMCONFIG) --ldflags --libs $(LLVMCOMPONENTS))

SOURCES = main.cpp astvisitor.cpp datatypes.cpp astvisitor_visits.cpp

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
	$(CXX) -g -o $@ $(OBJECTS) $(CLANGLIBS) $(LLVMLDFLAGS)

clean:
	-rm -f $(EXES) $(OBJECTS) *~
