#pragma "once"
#include <unordered_set>

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

            Project::Commit const * c = latestMasterCommit(p);

            emitLatestFiles(c, p);



            return;
        } catch (char const * msg) {
            std::cerr << p << ":" << msg << std::endl;
        } catch (...) {

        }
        ++errors_;

    }

    Project::Commit const * latestMasterCommit(Project const & p) {
        Project::Commit const * result = nullptr;
        for (auto const & bs : p.branchSnapshots) {
            if (bs.branch != "origin/master")
                continue;
            auto i = p.commits.find(Project::Commit(bs.commit, 0, ""));
            if (i == p.commits.end())
                throw "Latest master commit not captured.";
            if (result == nullptr || i->time > result->time)
                result = &(*i);
        }
        if (result == nullptr)
            throw "Project does not have master branch";
        return result;
    }

    void emitLatestFiles(Project::Commit const * c, Project const & p) {
        std::unordered_set<std::string> visited;
        while (true) {
            for (Project::Snapshot const & s : p.snapshots) {
                if (s.commit == c->commit) {
                    if (visited.insert(s.relPath).second) {
                        if (s.contentId != -1) {
                            snapshotFiles_ << p.id() << ","
                                           << s.id << ","
                                           << s.contentId << ","
                                           << escape(s.relPath) << ","
                                           << c->time << std::endl;
                        }
                    }
                }
            }
            auto i = p.commits.find(Project::Commit(c->parent, 0, ""));
            if (i == p.commits.end())
                break;
            c = & *i;
        }
    }





    long numProjects_;
    long errors_;
    std::ofstream snapshotFiles_;





};
