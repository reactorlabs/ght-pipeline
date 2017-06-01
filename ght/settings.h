#pragma once

#include <mutex>
#include "include/utils.h"


//#include "include/settings.h"


class Settings {
public:
    class General {
    public:
        static std::string Target;
        static bool Incremental;
        static unsigned NumThreads;
        static long DebugLimit;
        static long DebugSkip;
        static std::vector<std::string> ApiTokens;

        /** Number of files per token to be used.

          This is to prevent long response times of the filesystems if too many files are in same directory. Setting this to 0 bypasses the hierarchy and stores all files and projects on same level.
         */
        static unsigned FilesPerFolder;




        static void LoadAPITokens(std::string const & from);

        static std::string const & GetNextApiToken();


    private:
        static unsigned apiTokenIndex_;
        static std::mutex apiTokenGuard_;
    };

    class Cleaner {
    public:
        static std::vector<std::string> InputFiles;
        static std::vector<std::string> AllowedLanguages;
        static bool AllowForks;
    };

    class CleanerAllLang {
    public:
        static std::string OutputFile;
    };

    class Downloader {
    public:
        static std::vector<std::string> AllowPrefix;
        static std::vector<std::string> AllowSuffix;
        static std::vector<std::string> AllowContents;
        static std::vector<std::string> DenyPrefix;
        static std::vector<std::string> DenySuffix;
        static std::vector<std::string> DenyContents;

        static bool CompressFileContents;
        static bool CompressInExtraThread;
        static int MaxCompressorThreads;
        static bool KeepRepos;
        /** If true, github metadata is stored including the HTTP headers which allows skipping unchanged projects in the future inceremental downloads.
         */
        static bool KeepMetadataHeaders;
    };

    class StrideMerger {
    public:
        static std::string Folder;
    };


};

/** Takes given id and produces a path from it that would make sure the MaxFilesPerDirectory limit is not broken assuming full utilization of the ids.
*/
inline std::string IdToPath(long id, std::string const & prefix = "") {
    // when files per folder is disabled
    if (Settings::General::FilesPerFolder == 0)
        return "";
    std::string result = "";
    // get the directory id first, which chops the ids into chunks of MaxEntriesPerDirectorySize
    long dirId = id / Settings::General::FilesPerFolder;
    // construct the path while dirId != 0
    while (dirId != 0) {
        result = STR("/" << prefix << std::to_string(dirId % Settings::General::FilesPerFolder) << result);
        dirId = dirId / Settings::General::FilesPerFolder;
    }
    return result;
}


inline bool IdPathFull(long id) {
    if (Settings::General::FilesPerFolder == 0)
        return false;
    return ((id + 1) % Settings::General::FilesPerFolder) == 0;
}


