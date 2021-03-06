#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "include/utils.h"
#include "include/worker.h"
#include "include/timer.h"
#include "include/filesystem.h"
#include "include/pattern_lists.h"
#include "include/hash.h"

#include "ght/settings.h"

#include "git.h"

class Project {
public:

    class Commit {
    public:
        /** Commit's hash. */
        std::string commit;

        /** Parent commit's hash. */
        std::string parent;

        /** Time of the commit */
        int time;

        Commit(Git::Commit const & c, std::string const & parent):
            commit(c.hash),
            parent(parent),
            time(c.time) {
        }

        Commit(std::string const & hash, int time, std::string const & parent):
            commit(hash),
            parent(parent),
            time(time) {
        }

        bool operator == (Commit const & other) const {
            return commit == other.commit;
        }

        struct Hash {
            std::size_t operator()(Commit const & x) const {
                return std::hash<std::string>()(x.commit);
            }
        };

        friend std::ostream & operator << (std::ostream & s, Commit const & c) {
            s << c.commit << ","
              << c.time  << ","
              << (c.parent.empty() ? "\"\"" : c.parent);
            return s;
        }

    };


    class Snapshot {
    public:
        /** Id of the snapshot */
        long id;
        /** Id of the contents. */
        long contentId;
        /** Id of the parent for this snapshot, not tracking name changes. */
        long parentId;
        /** Commit hash */
        std::string commit;
        /** Relative path */
        std::string relPath;


        Snapshot(long id, std::string relPath, Commit const & c):
            id(id),
            contentId(-1),
            parentId(-1),
            commit(c.commit),
            relPath(relPath) {
        }

        Snapshot(long id, long contentId, long parentId, std::string const & commit, std::string const & relPath):
            id(id),
            contentId(contentId),
            parentId(parentId),
            commit(commit),
            relPath(relPath) {
        }

        bool operator == (Snapshot const & other) const {
            return commit == other.commit and relPath == other.relPath;
        }

        struct Hash {
            std::size_t operator()(Snapshot const & x) const {
                return std::hash<std::string>()(x.commit) + std::hash<std::string>()(x.relPath);
            }
        };

        friend std::ostream & operator << (std::ostream & s, Snapshot const & x) {
            s << x.id << ","
              << x.contentId << ","
              << x.parentId << ","
              << x.commit << ","
              << escape(x.relPath);
            return s;
        }
    };

    std::string gitUrl() const {
        return STR("https://github.com/" << url_ << ".git");
    }

    std::string apiUrl() const {
        return STR("https://api.github.com/repos/" << url_);
    }

    long id() const {
        return id_;
    }

    void initialize();

    bool loadPreviousRun();

    void clone(bool force = false);

    void loadMetadata();

    void deleteRepo();

    void finalize();




    void analyze(PatternList const & filter);

    Project(long id);
    Project(std::string const & relativeUrl);
    Project(std::string const & relativeUrl, long id);

    std::string fileLog() const {
        return STR(path_ << "/log.csv");
    }

    std::string fileBranches() const {
        return STR(path_ << "/branches.csv");
    }

    std::string fileCommits() const {
        return STR(path_ << "/commits.csv");
    }

    std::string fileSnapshots() const {
        return STR(path_ << "/snapshots.csv");
    }

    std::string fileBranchSnapshots() const {
        return STR(path_ << "/branchSnapshots.csv");
    }

    std::unordered_set<Commit, Commit::Hash> commits;
    std::vector<Snapshot> snapshots;


private:
    friend class Downloader;

    friend std::ostream & operator << (std::ostream & s, Project const & p) {
        s << p.id_ << ","
          << escape(p.url_) << ","
          << (p.hasDeniedFiles_ ? "1" : "0") << ","
          << p.resumeTime_ << ","
          << p.cloneTime_ << ","
          << p.metadataTime_ << ","
          << p.snapshotsTime_ << ","
          << p.deleteTime_;
        return s;
    }


    /* TODO The thing is how to know last id's, we can traverse the existing snapshots

      -> we must still download them from incremental

      ! but how to store them?
      */

    void analyzeCommit(PatternList const & filter, Commit const & c, std::string const & parent, std::ostream & fSnapshots);


    long id_;
    std::string url_;
    bool hasDeniedFiles_;

    bool shouldSkip_;


    std::string path_;
    std::string repoPath_;

    double resumeTime_;
    double cloneTime_;
    double metadataTime_;
    double snapshotsTime_;
    double deleteTime_;

    std::unordered_map<std::string, long> lastIds_;


    static std::atomic<long> idCounter_;






};




//} // namespace xx



class Downloader : public Worker<Downloader, Project> {
public:
    static std::vector<std::string> AllowPrefix;
    static std::vector<std::string> AllowSuffix;
    static std::vector<std::string> AllowContents;
    static std::vector<std::string> DenyPrefix;
    static std::vector<std::string> DenySuffix;
    static std::vector<std::string> DenyContents;
    static bool CompressFileContents;
    static bool CompressInExtraThread;
    static int CompressorThreads;
    static bool KeepRepos;
    static bool Incremental;
    static unsigned NumThreads;
    static std::string OutputDir;
    static std::string InputFile;
    static long DebugSkip;
    static long DebugLimit;

    static void ParseCommandLine(std::vector<std::string> const & args);


    static void Initialize();

    static void LoadPreviousRun();

    static void OpenOutputFiles();

    /** Reads the given file, and schedules each project in it for the download.

      The file should contain a git url per line.
     */
    static void FeedFrom(std::string const & filename);

    static void Finalize();

    static ProgressReporter::Feeder GetReporterFeeder();

    static long AssignContentId(SHA1 const & hash, std::string const & relPath, std::string const & root);

    std::string status() {
        if (currentJob_ == ' ')
            return "IDLE";
        else
            return STR(currentJob_ << ":" << currentProject_);
    }

private:

    friend class Project;

    static void CompressFiles(std::string const & targetDir);

    void run(Project & p) override {
        try {
            Timer t;
            currentProject_ = p.id_;
            currentJob_ = 'I'; // initialize
            p.initialize();
            // resume
            // update the project according to the previous run
            if (Incremental) {
                currentJob_ = 'R';
                p.loadPreviousRun();
            }
            p.resumeTime_ = t.seconds(true);
            // clone
            currentJob_ = 'C';
            p.clone(true);
            p.cloneTime_= t.seconds(true);
            // metadata
            currentJob_ = 'M';
            p.loadMetadata();
            p.metadataTime_ = t.seconds(true);
            // snapshots
            // look into all branches and download all file snapshots
            currentJob_ = 'S';
            p.analyze(language_);
            p.snapshotsTime_ = t.seconds(true);
            // writeback
            currentJob_ = 'W';
            {
                // make sure we flush the actual files mapping before writing the project to be sure we always end up in consistent state
                std::lock_guard<std::mutex> g(contentFileGuard_);
                contentHashesFile_.flush();
            }
            p.finalize();
            // delete
            if (not Downloader::KeepRepos) {
                currentJob_ = 'D';
                p.deleteRepo();
            }
            p.deleteTime_ = t.seconds();
            // nothing to do
            currentProject_ = -1;
            currentJob_ = ' '; // idle
        } catch (...) {
            currentJob_ = 'E';
            std::lock_guard<std::mutex> g(failedProjectsGuard_);
            failedProjectsFile_ << escape(p.gitUrl()) << "," << p.id_ << std::endl;
            try {
                p.deleteRepo();
            } catch (...) {
                // do nothing
            }
            throw;
        }
    }

    /** Current project id, for reporting purposes.
     */
    long currentProject_ = -1;

    /** Current job, for reporting purposes.
     */
    char currentJob_ = ' ';


    /** Where we write failed projects information
     */
    static std::ofstream failedProjectsFile_;

    static std::ofstream contentHashesFile_;

    static std::unordered_map<SHA1,long> contentHashes_;

    static std::mutex contentGuard_;
    static std::mutex failedProjectsGuard_;
    static std::mutex contentFileGuard_;


    static PatternList language_;

    static std::atomic<long> bytes_;
    static std::atomic<long> snapshots_;

    static std::atomic<int> compressors_;

};




