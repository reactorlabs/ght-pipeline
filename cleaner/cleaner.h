#pragma once

#include <iostream>
#include <string>
#include <unordered_set>

#include "include/csv.h"
#include "include/worker.h"
#include "include/filesystem.h"
#include "include/timer.h"

#include "ght/settings.h"



class Cleaner : public Worker<Cleaner, std::string> {
public:
    static std::vector<std::string> InputFiles;
    static std::vector<std::string> AllowedLanguages;
    static bool AllowForks;
    static bool Incremental;
    static std::string OutputFile;
    static long DebugSkip;
    static long DebugLimit;

    static void ParseCommandLine(std::vector<std::string> const & args);

    static void LoadPreviousRun() {
        if (not Incremental)
            return;
        if (not isFile(OutputFile)) {
            std::cout << "No previous run found" << std::endl;
        } else {
            std::cout << "Loading previous run" << std::endl;
            CSVParser p(OutputFile);
            for (auto row : p)
                projects_.insert(row[0]);
            std::cout << "    " << projects_.size() << " existing projects added.";
        }
    }

    static void FeedFrom(std::vector<std::string> const & inputs) {
        for (auto i : inputs) {
            std::cout << i << std::endl;
            Schedule(i);
        }
    }

    static void Finalize() {
    }

    static long SkippedProjects() {
        return skipped_;
    }

    static long AddedProjects() {
        return added_;
    }

    static long TotalProjects() {
        return total_;
    }

    static ProgressReporter::Feeder GetReporterFeeder() {
        return [](ProgressReporter & p, std::ostream & s) {
            p.done = added_;
            if (AllDone())
                p.allDone = true;
            s << "added: " << std::left << std::setw(8) << added_
              << "skipped: " << std::setw(8) << skipped_
              << "total: " << std::setw(8) << total_ << std::endl;
        };
    }

private:


    static bool IsForked(std::vector<std::string> const & row) {
        return row[7] != "\\N";
    }

    static bool IsDeleted(std::vector<std::string> const & row) {
        return row[8] != "0";
    }

    static std::string const & Language(std::vector<std::string> const & row) {
        return row[5];
    }

    static std::string RelativeUrl(std::vector<std::string> const & row) {
        return row[1].substr(29);
    }

    static bool IsValidLanguage(std::string const & language) {
        if (AllowedLanguages.empty())
            return true;
        for (std::string const & l : AllowedLanguages)
            if (l == language)
                return true;
        return false;
    }

    void run(std::string & filename) override {
        std::ofstream outFile(OutputFile, Incremental ? (std::fstream::out | std::fstream::app) : std::fstream::out);
        CSVParser p(filename);
        for (auto row : p) {
            while (DebugSkip > 0) {
                --DebugSkip;
                continue;
            }
            if (DebugLimit != -1 and total_ >= DebugLimit)
                break;
            if (not IsDeleted(row)) {
                if (AllowForks or not IsForked(row)) {
                    if (IsValidLanguage(Language(row))) {
                        std::string url = RelativeUrl(row);
                        // store the project to the outfile only if we haven't yet seen the url
                        if (projects_.insert(url).second) {
                            outFile << url << std::endl;
                            ++added_;
                        } else {
                            ++skipped_;
                        }
                    }
                }
            }
            ++total_;
        }
    }

    static long skipped_;
    static long added_;
    static long total_;

    static std::unordered_set<std::string> projects_;

};







