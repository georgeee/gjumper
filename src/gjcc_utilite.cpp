#include "processor.h"
#include <cstring>

static const char * const GJ_DIR_ARG = "--gj_dir=";

int main(int argc, char **argv)
{
#ifdef GJUMPER_GJCC_CXX
    bool isCxx = true;
#endif
#ifdef GJUMPER_GJCC_C
    bool isCxx = false;
#endif
    std::string project_config = PROJECT_COMPILE_CONFIG_NAME;
    std::string baseDir = ".";
    argv++; argc --;
    if(argc > 0 && strncmp(argv[0], GJ_DIR_ARG, strlen(GJ_DIR_ARG)) == 0){
        baseDir = argv[0] + strlen(GJ_DIR_ARG);
        argv++; argc --;
    }
    gj::addToCompileConfig(isCxx, argv, argc, baseDir);
    return 1;
}
