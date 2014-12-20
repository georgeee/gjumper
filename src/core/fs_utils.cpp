#include "fs_utils.h"

namespace boost{
    namespace filesystem{
        template <>
            path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end, const codecvt_type& cvt){ 
                for( ; begin != end ; ++begin )
                    *this /= *begin;
                return *this;
            }
        // Return path when appended to parent will resolve to same as child
        path relativize(path parent, path child){
            parent = canonical(parent);
            child = canonical(child);

            // Find common base
            path::const_iterator parent_iter(parent.begin()), child_iter(child.begin());
            path::const_iterator child_end(child.end()), parent_end(parent.end());
            while(parent_iter != parent_end && child_iter != child_end && *parent_iter == *child_iter)
                ++parent_iter, ++child_iter;

            // Navigate backwards in directory to reach previously found base
            path ret;
            while(parent_iter != parent_end)
            {
                if( (*parent_iter) != "." )
                    ret /= "..";
                ++parent_iter;
            }

            // Now navigate down the directory branch
            ret.append(child_iter, child.end());
            return ret;
        }
    }
}
