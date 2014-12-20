#include <boost/filesystem.hpp>

#ifndef GJ_FSUTILS
#define GJ_FSUTILS

namespace boost{
    namespace filesystem{
        // Return path when appended to parent will resolve to same as child
        path relativize(path parent, path child);
    }
}

#endif

