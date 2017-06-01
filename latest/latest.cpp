#include "latest.h"
#include "include/settings.h"

std::string Latest::OutputDir;

void Latest::ParseCommandLine(std::vector<std::string> const & args) {
    Settings s;
    s.addOption("-o", OutputDir, true, "Output directory where the input.csv file will be created");
    s.parseCommandLine(args, 2);
}
