#pragma once

#include <mutex>
#include "include/utils.h"

class FolderHierarchy {
public:
    static unsigned FilesPerFolder;

    /** Takes given id and produces a path from it that would make sure the MaxFilesPerDirectory limit is not broken assuming full utilization of the ids.
    */
    static std::string IdToPath(long id, std::string const & prefix = "") {
        // when files per folder is disabled
        if (FilesPerFolder == 0)
            return "";
        std::string result = "";
        // get the directory id first, which chops the ids into chunks of MaxEntriesPerDirectorySize
        long dirId = id / FilesPerFolder;
        // construct the path while dirId != 0
        while (dirId != 0) {
            result = STR("/" << prefix << std::to_string(dirId % FilesPerFolder) << result);
            dirId = dirId / FilesPerFolder;
        }
        return result;
    }


    static bool IdPathFull(long id) {
        if (FilesPerFolder == 0)
            return false;
        return ((id + 1) % FilesPerFolder) == 0;
    }


};
