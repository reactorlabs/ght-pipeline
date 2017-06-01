#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <string>

#include "ght/settings.h"


#include "cleaner/cleaner.h"
#include "cleaner/alllang.h"
#include "downloader/downloader.h"
#include "include/settings.h"

#include "sccsorter/sccsorter.h"
#include "latest/latest.h"





unsigned FolderHierarchy::FilesPerFolder = 1000;

void Clean(std::vector<std::string> const & args) {
    Cleaner::ParseCommandLine(args);
    Cleaner::LoadPreviousRun();
    Cleaner::Spawn(1);
    ProgressReporter::Start(Cleaner::GetReporterFeeder());
    Cleaner::Run();
    Cleaner::FeedFrom(Cleaner::InputFiles);
    Cleaner::Wait();
    Cleaner::Finalize();
}

void Download(std::vector<std::string> const & args) {
    Downloader::ParseCommandLine(args);
    Downloader::Initialize();
    Downloader::LoadPreviousRun();
    Downloader::OpenOutputFiles();
    Downloader::Spawn(Downloader::NumThreads);
    ProgressReporter::Start(Downloader::GetReporterFeeder());
    Downloader::Run();
    Downloader::FeedFrom(Downloader::InputFile);
    Downloader::Wait();
    Downloader::Finalize();
}

void Latest(std::vector<std::string> const & args) {
    Latest::ParseCommandLine(args);
    Latest::GetLatestSnapshot();
}

void CleanAllLang(std::vector<std::string> const & args) {
    CleanerAllLang::ParseCommandLine(args);
    CleanerAllLang::Spawn(1);
    ProgressReporter::Start(CleanerAllLang::GetReporterFeeder());
    CleanerAllLang::Run();
    CleanerAllLang::FeedFrom(Cleaner::InputFiles);
    CleanerAllLang::Wait();
}

int main(int argc, char * argv[]) {
    try {
        std::vector<std::string> args = Settings::ReadCommandLine(argc, argv);
        if (argc < 2)
            throw std::invalid_argument("Not enough arguments");
        std::string command = args[1];
        if (command == "cleaner")
            Clean(args);
        else if (command == "cleanerAllLang")
            CleanAllLang(args);
        else if (command == "downloader")
            Download(args);
        else if (command == "latest")
            Latest(args);
        else
            throw std::invalid_argument(STR("Invalid command " << command));
        std::cout << "DONE" << std::endl;
        return EXIT_SUCCESS;
    } catch (std::exception const & e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
