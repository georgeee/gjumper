CXX := clang++
LLVMCOMPONENTS := cppbackend
RTTIFLAG := #-fno-rtti
LLVMCONFIG := /usr/bin/llvm-config-3.3

CXXFLAGS := -g -I$(shell $(LLVMCONFIG) --src-root)/tools/clang/include -I$(shell $(LLVMCONFIG) --obj-root)/tools/clang/include $(shell $(LLVMCONFIG) --cxxflags) $(RTTIFLAG) -Wno-c++11-extensions -std=c++11 -fexceptions
LLVMLDFLAGS := $(shell $(LLVMCONFIG) --ldflags --libs $(LLVMCOMPONENTS))
LINKFLAGS := -ljsoncpp -lboost_filesystem -lboost_system

SOURCES = astvisitor.cpp datatypes.cpp astvisitor_visits.cpp hintdb_exporter.cpp processor.cpp hierarcy_tree.cpp hint_db_cache_manager.cpp communication.cpp

OBJECTS = $(SOURCES:.cpp=.o)
EXE_CPPS = full_recacher.cpp gjumper_client.cpp
EXE_OS = gjcc_utilite_cxx.o gjcc_utilite_c.o $(EXE_CPPS:.cpp=.o)
EXES = $(EXE_OS:.o=)
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
			-lclangEdit\
			-lclangAST\
			-lclangLex\
			-lclangBasic\
			$(shell $(LLVMCONFIG) --libs)\
			-lcurses

all: $(OBJECTS) $(EXES)

#$(EXE_CPPS:.cpp=) : $(OBJECTS) $(EXE_CPPS)
	#$(CXX) -g -o $@ $(OBJECTS) $@.cpp $(CXXFLAGS) $(CLANGLIBS) $(LLVMLDFLAGS) $(LINKFLAGS)

#full_recacher: $(OBJECTS) full_recacher.o
	#$(CXX) -g -o $@ $(OBJECTS) full_recacher.o $(CLANGLIBS) $(LLVMLDFLAGS) $(LINKFLAGS)

$(EXE_OS:.o=) : $(OBJECTS) $(EXE_OS)
	$(CXX) -g -o $@ $(OBJECTS) $@.o $(CLANGLIBS) $(LLVMLDFLAGS) $(LINKFLAGS)

#gjccxx: $(OBJECTS) gjcc_utilite_cxx.o
	#$(CXX) -g -o $@ $(OBJECTS) gjcc_utilite_cxx.o $(CLANGLIBS) $(LLVMLDFLAGS) $(LINKFLAGS)
#gjcc: $(OBJECTS) gjcc_utilite_c.o
	#$(CXX) -g -o $@ $(OBJECTS) gjcc_utilite_c.o $(CLANGLIBS) $(LLVMLDFLAGS) $(LINKFLAGS)

gjcc_utilite_cxx.o : gjcc_utilite.cpp
	$(CXX) -g -c -o $@ -DGJUMPER_GJCC_CXX $(CXXFLAGS) $<
gjcc_utilite_c.o : gjcc_utilite.cpp
	$(CXX) -g -c -o $@ -DGJUMPER_GJCC_C $(CXXFLAGS) $<

clean:
	-rm -f $(EXES) $(OBJECTS) *~ $(EXE_OS)
