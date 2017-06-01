#include "downloader.h"
#include "github.h"

#include "include/csv.h"
#include "include/exec.h"
#include "include/settings.h"


#include "git.h"


std::vector<std::string> Downloader::AllowPrefix = {};
std::vector<std::string> Downloader::AllowSuffix = {};
std::vector<std::string> Downloader::AllowContents = {};
std::vector<std::string> Downloader::DenyPrefix = {};
std::vector<std::string> Downloader::DenySuffix = {};
std::vector<std::string> Downloader::DenyContents = {};

bool Downloader::CompressFileContents = true;
int Downloader::CompressorThreads = 2;
bool Downloader::KeepRepos = true;
bool Downloader::Incremental = false;
unsigned Downloader::NumThreads = 4;
std::string Downloader::OutputDir;
std::string Downloader::InputFile;
long Downloader::DebugSkip = 0;
long Downloader::DebugLimit = -1;

std::atomic<long> Project::idCounter_(0);


std::atomic<long> Downloader::snapshots_(0);


void Downloader::ParseCommandLine(std::vector<std::string> const & args) {
    Settings s;
    s.addOption("+p", AllowPrefix, false, "Files with given prefix will be downloaded.");
    s.addOption("+s", AllowSuffix, false, "Files with given suffix (extension) will be downloaded.");
    s.addOption("+c", AllowContents, false, "Files containing given string will be downloaded.");
    s.addOption("-p", DenyPrefix, false, "Files with given prefix will be ignored.");
    s.addOption("-s", DenySuffix, false, "Files with given suffix will be ignored.");
    s.addOption("-c", DenyContents, false, "Files containing given string will be ignored.");
    s.addOption("-compress", CompressFileContents, false, "If true, output files in folder will be compressed when the folder is full");
    s.addOption("-compressorThreads", CompressorThreads, false, "Determines the number of compressor threads available, 0 for compressing in downloader thread.");
    s.addOption("-keepRepos", KeepRepos, false, "If true, repository contents will not be deleted when the analysis is finished.");

    s.addOption("-apiToken", Github::ApiTokens, false, "Github API Tokens which are rotated for the requests made to circumvent github api limitations");

    s.addOption("-maxFolderSize", FolderHierarchy::FilesPerFolder, false, "Maximum number of files and directories in a single folder in the output. 0 for unlimited.");

    s.addOption("-incremental", Incremental, false, "If true, the download will be incremental, i.e. keep already downloaded data");

    s.addOption("-threads", NumThreads, false, "Number of concurrent threads the downloader is allowed to use");

    s.addOption("-o", OutputDir, true, "Output directory where the downloaded projects & files will be stored. Also the directory that must contain the input file input.csv");

    s.addOption("-debugSkip", DebugSkip, false, "Number of projects at the beginning of first input file to skip");
    s.addOption("-debugLimit", DebugLimit, false, "Max number of projects to emit");
    s.addOption("-input", InputFile, true, "Input file with project addresses on github");

    s.parseCommandLine(args, 2);
}

void Project::initialize() {
    createPathIfMissing(path_);

}

bool Project::loadPreviousRun() {
    try {
        CSVParser c(fileCommits());
        for (auto & row : c)
            commits.insert(Commit(row[0], std::stoi(row[1]), row[2]));
        CSVParser fs(fileSnapshots());
        for (auto & row : fs)
            snapshots.push_back(Snapshot(std::stol(row[0]), std::stol(row[1]), std::stol(row[2]), row[3], row[4]));
    } catch (...) {
        return false;
    }
}

void Project::clone(bool force) {
    if (isDirectory(repoPath_)) {
        if (force) {
            deletePath(repoPath_);
        } else {
            // TODO pull instead of clone
            return;
        }
    }
    if (not Git::Clone(gitUrl(), repoPath_))
        throw std::runtime_error(STR("Unable to download project " << gitUrl() << ", id " << id_));
}

void Project::loadMetadata() {
    // no tokens, no metadata
    if (Github::ApiTokens.empty())
        return;
    // get the current API token (remember we are rotating them)
    std::string const & token = Github::NextApiToken();
    // construct the API request
    std::string req = STR("curl -D metadata.headers -s " << apiUrl() << " -H \"Authorization: token " << token << "\" -o metadata.json");
    // exec, curl already splits headers & contents
    exec(req, path_);
}

void Project::deleteRepo() {
    deletePath(repoPath_);
}

void Project::finalize() {
    std::ofstream fLog = CheckedOpen(fileLog(), Downloader::Incremental);
    fLog << *this << std::endl;
}

void Project::analyze(PatternList const & filter) {
    // open the output streams
    std::ofstream fCommits = CheckedOpen(fileCommits(), Downloader::Incremental);
    std::ofstream fSnapshots = CheckedOpen(fileSnapshots(), Downloader::Incremental);
    // use the current branch as main one and get all its commits
    std::vector<Git::Commit> commits = Git::GetCommits(repoPath_);

    // parent commit
    std::string parent = "";
    // start from back because git returns the commits starting from latest, but we want from oldest
    for (auto i = commits.rbegin(), e = commits.rend(); i != e; ++i) {
        Commit c(*i, parent);
        // if we haven't yet seen the commit, report & analyze it, do nothing if we have seen it
        if (this->commits.insert(c).second) {
            fCommits << c << std::endl;
            analyzeCommit(filter, c, parent, fSnapshots);
        }
        // update the parent
        parent = c.commit;
    }
}

void Project::analyzeCommit(PatternList const & filter, Commit const & c, std::string const & parent, std::ostream & fSnapshots) {
    bool checked = false;
    for (auto obj : Git::GetObjects(repoPath_, c.commit, parent)) {
        // check if it is a language file
        if (not filter.check(obj.relPath, hasDeniedFiles_))
            continue;
        Snapshot s(snapshots.size(), obj.relPath, c);
        // set the parent id if we have one, keep -1 if not
        auto i = lastIds_.find(s.relPath);
        if (i != lastIds_.end())
            s.parentId = i->second;
        if (obj.type == Git::Object::Type::Deleted) {
            s.contentId = -1;
            lastIds_[s.relPath] = -1;
        } else {
            if (not checked) {
                Git::Checkout(repoPath_, c.commit);
                checked = true;
            }
            s.contentId = Downloader::AssignContentId(SHA1(obj.hash), obj.relPath, repoPath_);
            lastIds_[s.relPath] = s.id;
        }
        snapshots.push_back(s);
        fSnapshots << s << std::endl;
        ++Downloader::snapshots_;
    }
}

Project::Project(long id):
    id_(id),
    url_("") {
    while (idCounter_ < id) {
        long old = idCounter_;
        if (idCounter_.compare_exchange_weak(old, id))
            break;
    }
    path_ = STR(Downloader::OutputDir << "/projects" << FolderHierarchy::IdToPath(id_, "projects_") << "/" << id_);
    repoPath_ = STR(path_ << "/repo");
}

Project::Project(std::string const & relativeUrl):
    id_(idCounter_++),
    url_(relativeUrl) {
    path_ = STR(Downloader::OutputDir << "/projects" << FolderHierarchy::IdToPath(id_, "projects_") << "/" << id_);
    repoPath_ = STR(path_ << "/repo");
}

Project::Project(std::string const & relativeUrl, long id):
    id_(id),
    url_(relativeUrl) {
    ++id;
    while (idCounter_ < id) {
        long old = idCounter_;
        if (idCounter_.compare_exchange_weak(old, id))
            break;
    }
    path_ = STR(Downloader::OutputDir << "/projects" << FolderHierarchy::IdToPath(id_, "projects_") << "/" << id_);
    repoPath_ = STR(path_ << "/repo");
}


std::ofstream Downloader::failedProjectsFile_;

std::ofstream Downloader::contentHashesFile_;

std::unordered_map<SHA1,long> Downloader::contentHashes_;

std::mutex Downloader::contentGuard_;
std::mutex Downloader::failedProjectsGuard_;
std::mutex Downloader::contentFileGuard_;

PatternList Downloader::language_;

std::atomic<long> Downloader::bytes_(0);
std::atomic<int> Downloader::compressors_(0);


// Downloader -------------------------------------------------------------------------------------

void Downloader::Initialize() {
    // fill in the language filter object
    for (auto i : Downloader::AllowPrefix)
        language_.allowPrefix(i);
    for (auto i : Downloader::AllowSuffix)
        language_.allowSuffix(i);
    for (auto i : Downloader::AllowContents)
        language_.allow(i);
    for (auto i : Downloader::DenyPrefix)
        language_.denyPrefix(i);
    for (auto i : Downloader::DenySuffix)
        language_.denySuffix(i);
    for (auto i : Downloader::DenyContents)
        language_.deny(i);

}

void Downloader::LoadPreviousRun() {
    // load the file contents
    if (not Incremental)
        return;
    std::string content_hashes = STR(OutputDir << "/content_hashes.csv");
    if (! isFile(content_hashes))
        return;
    std::cout << "Loading content hashes from previous runs" << std::flush;

    CSVParser p(content_hashes);
    for (auto i : p) {
        SHA1 hash(i[0]);
        long id = std::stol(i[1]);
        contentHashes_.insert(std::make_pair(hash, id));
        if (contentHashes_.size() % 10000000 == 0)
            std::cout << "." << std::flush;
    }
    std::cout << std::endl << "    " << contentHashes_.size() << " ids loaded" << std::endl;
}

void Downloader::OpenOutputFiles() {
    // open the streams
    // TODO should the failed projects be suffixed with run id?
    failedProjectsFile_ = CheckedOpen(STR(OutputDir << "/failed_projects.csv"));
    contentHashesFile_ = CheckedOpen(STR(OutputDir << "/content_hashes.csv"), Incremental);
}

void Downloader::FeedFrom(std::string const & filename) {
    CSVParser p(filename);
    long line = 1;
    long i = 0;
    for (auto x : p) {
        if (DebugSkip > 0) {
            --DebugSkip;
            ++line;
            continue;
        }
        ++line;
        if (DebugLimit != -1 and i >= DebugLimit)
            break;
        ++i;
        if (x.size() == 1) {
            if (x[0].find("http://github.com/") == 0)
                x[0] = x[0].substr(18, x[0].size() - 18 - 4); // and .git
            Project p(x[0]);
            if (not isFile(p.fileLog()))
                Schedule(p);
            continue;
        } else if (x.size() == 2) {
            try {
                char ** c;
                if (x[0].find("http://github.com/") == 0)
                    x[0] = x[0].substr(18, x[0].size() - 18 - 4); // and .git
                Project p(x[0], std::strtol(x[1].c_str(), c, 10));
                if (not isFile(p.fileLog()))
                    Schedule(p);
                continue;
            } catch (...) {
                // the code below outputs the error too
            }
        }
        Error(STR(filename << ", line " << line << ": Invalid format of the project url input, skipping."));
    }
}

void Downloader::Finalize() {
    failedProjectsFile_.close();
    contentHashesFile_.close();
    std::ofstream stamp = CheckedOpen(STR(OutputDir << "/runs_downloader.csv"), Incremental);
    stamp << Timer::SecondsSinceEpoch() << ","
          << Project::idCounter_ << ","
          << ErrorTasks() << ","
          << contentHashes_.size() << ","
          << snapshots_ << ","
          << TotalTime() << std::endl;
    // a silly busy wait
    while (compressors_ > 0) {
    }
}


ProgressReporter::Feeder Downloader::GetReporterFeeder() {
    return [](ProgressReporter & p, std::ostream & s) {
        p.done = CompletedTasks();
        p.errors = ErrorTasks();
        if (AllDone())
            p.allDone = true;
        int i = 0;
        int j = 0;
        for (Downloader * d : threads_) {
            if (d != nullptr) {
                s << "[" << std::right << std::setw(2) << i << "]" << std::left << std::setw(16) << d->status() << " ";
                if (++j == 4) {
                    j = 0;
                    s << std::endl;
                }
            }
            ++i;
        }
        if (j != 0)
            s << std::endl;
        s << "total bytes: " << std::setw(10) << std::left << Bytes(bytes_);
        s << "unique files: " << std::setw(16) << std::left << contentHashes_.size();
        s << "snapshots: " << std::setw(16) << std::left << snapshots_;
        s << "active compressors " << compressors_ << std::endl;
    };
}

long Downloader::AssignContentId(SHA1 const & hash, std::string const & relPath, std::string const & root) {
    long id;
    {
        std::lock_guard<std::mutex> g(contentGuard_);
        auto i = contentHashes_.find(hash);
        if (i != contentHashes_.end())
            return i->second;
        id = contentHashes_.size();
        contentHashes_.insert(std::make_pair(hash, id));
    }
    std::string contents = LoadEntireFile(STR(root << "/" << relPath));
    bytes_ += contents.size();
    // we have a new hash now, the file contents must be stored and the contents hash file appended
    std::string targetDir = STR(OutputDir << "/files" << FolderHierarchy::IdToPath(id, "files_"));
    createPathIfMissing(targetDir);
    {
        std::ofstream f = CheckedOpen(STR(targetDir << "/" << id << ".raw"));
        f << contents;
    }
    // output the mapping
    {
        std::lock_guard<std::mutex> g(contentFileGuard_);
        contentHashesFile_ << hash << "," << id << std::endl;
    }
    // compress the folder if it is full
    if (CompressFileContents and FolderHierarchy::IdPathFull(id)) {
        if (compressors_ < Downloader::CompressorThreads) {
            std::thread t([targetDir] () {
                CompressFiles(targetDir);
            });
            t.detach();
        } else {
            CompressFiles(targetDir);
        }
    }
    return id;
}

void Downloader::CompressFiles(std::string const & targetDir) {
    try {
        ++compressors_;
        bool ok = true;
        ok = exec("tar cfJ files.tar.xz *.raw", targetDir);
        ok = exec("rm -f *.raw", targetDir);
        if (not ok)
            std::cerr << "Unable to compress files in " << targetDir << std::endl;
    } catch (...) {
    }
    --compressors_;
}



