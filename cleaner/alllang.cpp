#include "alllang.h"
#include "include/settings.h"


std::string CleanerAllLang::OutputFile;

long CleanerAllLang::skipped_ = 0;
long CleanerAllLang::added_ = 0;
long CleanerAllLang::total_ = 0;

std::unordered_set<std::string> CleanerAllLang::projects_;

void CleanerAllLang::ParseCommandLine(std::vector<std::string> const & args) {
    Settings s;
    s.addOption("-o", OutputFile, true, "Output file where the input.csv file will be created");
    s.parseCommandLine(args, 2);
}
