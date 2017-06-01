#include "include/settings.h"

#include "cleaner.h"


std::vector<std::string> Cleaner::InputFiles;
std::vector<std::string> Cleaner::AllowedLanguages;
bool Cleaner::AllowForks = false;
bool Cleaner::Incremental = false;
std::string Cleaner::OutputDir;
long Cleaner::DebugSkip = 0;
long Cleaner::DebugLimit = -1;



long Cleaner::skipped_ = 0;
long Cleaner::added_ = 0;
long Cleaner::total_ = 0;

std::unordered_set<std::string> Cleaner::projects_;

void Cleaner::ParseCommandLine(std::vector<std::string> const & args) {
    Settings s;
    s.addOption("-l", AllowedLanguages, true, "Name of project language that should be filtered by the cleaner. Can be used multiple times to filter more than one language. Only filtered languages will be in the output");
    s.addOption("-f", InputFiles, true, "Input files in ghtorrent format from which the projects will be filtered.");

    s.addOption("-forks", AllowForks, false, "Determines whether forked projects will be included in the cleaner output, or not. Forks are not included by default.");

    s.addOption("-incremental", Incremental, false, "If incremental, only new projects will be added to the output. Non-incremental clean overwrites previous results");

    s.addOption("-o", OutputDir, true, "Output directory where the input.csv file will be created");

    s.addOption("-debugSkip", DebugSkip, false, "Number of projects at the beginning of first input file to skip");
    s.addOption("-debugLimit", DebugLimit, false, "Max number of projects to emit");

    s.addDefaultOption("-f");

    s.parseCommandLine(args, 2);
}

