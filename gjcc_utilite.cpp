#include "processor.h"

int main(int argc, char **argv)
{
#ifdef GJUMPER_GJCC_CXX
    bool isCxx = true;
#endif
#ifdef GJUMPER_GJCC_C
    bool isCxx = false;
#endif
    gj::addToCompileConfig(isCxx, argv + 1, argc - 1);
    return 1;
}
