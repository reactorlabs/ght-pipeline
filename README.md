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

## Usage

First argument is a command, followed by arguments for the particular command. The following lists the commands and their respective arguments. The arguments are given as command line args using the notation `name=value`

### `config`

The config command must be followed by a filename, from which the actual command line arguments will be read, one argument per line, empty lines and comments (lines starting with `#` are ignored).

### `cleaner`

Filters only given projects from the ghtorrent snapshot.

- `-l"` Name of project language that should be filtered by the cleaner. Can be used multiple times to filter more than one language. Only filtered languages will be in the output
- `-f` Input files in ghtorrent format from which the projects will be filtered
- `-forks` Determines whether forked projects will be included in the cleaner output, or not. Forks are not included by default
- `-incremental` If incremental, only new projects will be added to the output. Non-incremental clean overwrites previous results
- `-o` Output directory where the input.csv file will be created
- `-debugSkip` Number of projects at the beginning of first input file to skip
- `-debugLimit` Max number of projects to emit

### `downloader`

Downloads the projects specified by cleaner.

- `+p` Files with given prefix will be downloaded
- `+s` Files with given suffix (extension) will be downloaded
- `+c` Files containing given string will be downloaded
- `-p` Files with given prefix will be ignored
- `-s` Files with given suffix will be ignored
- `-c` Files containing given string will be ignored
- `-compress` If true, output files in folder will be compressed when the folder is full
- `-compressorThreads` Determines the number of compressor threads available, 0 for compressing in downloader thread
- `-keepRepos` If true, repository contents will not be deleted when the analysis is finished
- `-apiToken` Github API Tokens which are rotated for the requests made to circumvent github api limitations
- `-maxFolderSize` Maximum number of files and directories in a single folder in the output. 0 for unlimited
- `-incremental` If true, the download will be incremental, i.e. keep already downloaded data
- `-threads` Number of concurrent threads the downloader is allowed to use
- `-o` Output directory where the downloaded projects & files will be stored. Also the directory that must contain the input file input.csv
- `-debugSkip` Number of projects at the beginning of first input file to skip
- `-debugLimit` Max number of projects to emit
- `-input` Input file with project addresses on github


### `latest`

Creates a snapshot of only the latest versions of files for projects downloaded by the downloader.

- `-o` Output directory where the input.csv file will be created



