# ght-pipeline

Collection of utilities for analyzing large code repositories. 

> This is work in progress and is not really user friendly - notably arguments are usually hardcoded. See respective main functions for details. 

## Build instructions

The project uses `cmake`, so when you clone it, you can do something like this:

    mkdir build
    cd build
    cmake ..
    make
    
> The downloader is a work in progress, many of the settings are hardcoded.

# Downloader

The dowloader produces for each project the following:

`commits.csv`

Contains all commits on the main branch in the following format:

- commit hash
- commit time
- parent commit hash

`snapshots.csv`

Contains file snapshots for any commits on the master branch in the following format:

- snapshot id (unique per project)
- contents id (globally unique, -1 means the file has been deleted in the commit
- parent snapshot id (unique per project), -1 means the file has been created in the commit
- file contents hash (SHA-1) as reported by git
- relative path of the file

`log.csv`

Contains log information for the downloader passes on the project. If incremental run is enabled, each downloader run appends a line to the log file.

- project id
- project rel path on github
- flag whether the project has blacklisted files
- time for the incremental resume phase
- time required to clone the project
- time required to download the metadata
- time required to analyze file snapshots
- time required to delete the project

`metadata.json`

Metadata in JSON format including headers, as obtained from github.














































# old stuff

You can now run the respective tools, i.e.:

    ./cleaner
    ./downloader

## Cleaner

Takes the `projects.csv` from GHTorrent and selects only projects that are specified by given language, are not reported as deleted and are not forks. The output of cleaner is a list of `git` urls for projects to be downloaded. 

> TODO for the download, we might want forks - they may still contain new interesting content. 

## Downloader

Downloader takes the input list of projects produced by `cleaner` and downloads the projects one by one. For each project, searches for all files in all branches and all revisions which conform to a given white- and black- lists. 

### Downloader Output

> Args are hardcoded, see `downloader/downloader.cpp` for more details. Note there is 10k project limit and 8 threads running in parallel now. 

In the `projects` folder, for each project creates `files.csv` which contains all versions of all files it has found. Each file version has its name, branch, commit, date and content index. The `branches.csv` contains for each branch the downloader has observed the revision at which it was checked and list of present files (ids to the `files.csv` table). 

In the `stats` folder, in `projects.csv` information about each project is stored. The `contents.csv` file contains mapping from file contents hash to its id. The `runs.csv` file contains information about downloader runs in the current output directory. 

> This is in preparation for the incremental runs. 

## File Hierarchy

To avoid large numbers of files or directories in the same directory which might slow down the system, the `settings.h` file defines max number of files per directory. When this number is exceeded, a subdirectory is created. Function to convert id to path is provided for convenience. 





