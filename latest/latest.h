#pragma "once"
#include <unordered_set>
#include <climits>

#include "include/csv.h"
#include "ght/settings.h"
#include "include/filesystem.h"
#include "downloader/downloader.h"

/* Get number of projects first.
 */
class Latest {
public:
    static void GetLatestSnapshot() {
        Latest l;
        l.analyze();
    }

private:

    Latest() {
        snapshotFiles_ = CheckedOpen(STR(Settings::General::Target << "/latest_master.csv"));
        CSVParser p(STR(Settings::General::Target << "/runs_downloader.csv"));
        numProjects_ = 0;
        for (auto row : p) {
            long p = std::stol(row[1]) - 1;
            if (p > numProjects_)
                numProjects_ = p;
        }
        std::cout << numProjects_ << " downloaded projects found" << std::endl;
    }

    void analyze() {
        errors_ = 0;
        for (long i = 0; i <= numProjects_; ++i) {
            Project p(i);
            analyzeProject(p);
        }
        std::cout << "Errors: " << errors_ << std::endl;
    }



    /* Analyzes i-th project. */
    void analyzeProject(Project & p) {
        try {
            if (! p.loadPreviousRun())
                throw "Bad project.";

            Project::Commit c = latestCommit(p);

            emitLatestFiles(c, p);



            return;
        } catch (char const * msg) {
            std::cerr << p << ":" << msg << std::endl;
        } catch (...) {

        }
        ++errors_;

    }

    Project::Commit latestCommit(Project const & p) {
        Project::Commit result("", INT_MAX, "");
        for (auto const & c : p.commits)
            if (result.time > c.time)
                result = c;
        return result;
    }

    void emitLatestFiles(Project::Commit c, Project const & p) {
        std::unordered_set<std::string> visited;
        while (true) {
            for (Project::Snapshot const & s : p.snapshots) {
                if (s.commit == c.commit) {
                    // if we haven't seen the file
                    if (visited.insert(s.relPath).second) {
                        // and if the file is not deleted by the commit, emit it
                        if (s.contentId != -1) {
                            snapshotFiles_ << p.id() << ","
                                           << s.id << ","
                                           << s.contentId << ","
                                           << escape(s.relPath) << ","
                                           << c.time << std::endl;
                        }
                    }
                }
            }
            // move to the parent commit, or terminate if there is no parent commit
            auto i = p.commits.find(Project::Commit(c.parent, 0, ""));
            if (i == p.commits.end())
                break;
            c = *i;
        }
    }

    long numProjects_;
    long errors_;
    std::ofstream snapshotFiles_;

};
