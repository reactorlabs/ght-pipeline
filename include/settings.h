#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <functional>
#include <climits>
#include <fstream>
#include <cstring>

#include "utils.h"

class Settings {
public:

    static std::vector<std::string> ReadCommandLine(int argc, char * argv[]) {
        std::vector<std::string> result;
        result.push_back(argv[0]);
        if (argc == 3 && strncmp(argv[1], "config", 7) == 0) {
            std::ifstream args(argv[2]);
            if (! args.good())
                throw std::invalid_argument(STR("Cannot open config file " << argv[2]));
            std::string line;
            while (std::getline(args, line)) {
                if (! line.empty() && line[0] != '#')
                    result.push_back(line);
            }
        } else {
            for (unsigned i = 1; i < argc; ++i)
                result.push_back(argv[i]);
        }
        return result;
    }

    void parseCommandLine(std::vector<std::string> const & args, unsigned startAt = 1) {
        for (unsigned i = startAt, e = args.size(); i != e; ++i) {
            std::string name;
            std::string value;
            parseArgument(args[i], name, value);
            auto j = options_.find(name);
            if (j == options_.end()) {
                if (defaultArgument_.parser != nullptr && value.empty())
                    defaultArgument_.parse(name);
                else
                    throw std::invalid_argument(STR("Unknown argument " << name));
            } else {
                j->second.parse(value);
            }
        }
        // check that all required arguments were supplied
        for (auto i : options_)
            if (i.second.required and ! i.second.supplied)
                throw std::invalid_argument(STR("Argument " << i.first << " not supplied"));
    }

    template<typename T>
    void addOption(std::string const & name, T & value, bool required, std::string const & description) {
        assert(options_.find(name) == options_.end() && "Option name already exists");
        T * ptr = & value;
        options_[name] = Record(description, required, [ptr](std::string const & from) { Settings::ParseValue<T>(from, *ptr); });
    }

    template<typename T>
    void addDefaultOption(T & value, std::string const & description) {
        assert(defaultArgument_.parser == nullptr && "Redefining default argument");
        T * ptr = & value;
        defaultArgument_ = Record(description, false, [ptr](std::string const & from) { Settings::ParseValue<T>(from, *ptr); });
    }

    void addDefaultOption(std::string const & name) {
        assert(defaultArgument_.parser == nullptr && "Redefining default argument");
        assert(options_.find(name) != options_.end() && "Option must exist");
        defaultArgument_ = options_[name];
    }

    void printHelp(std::ostream & s) {
        // TODO when I have time

    }

private:
    template<typename T>
    static void ParseValue(std::string const & from, T & value);

    typedef std::function<void(std::string const &)> ParserFunction;

    class Record {
    public:
        std::string description;
        bool required;
        bool supplied;
        ParserFunction parser;

        Record():
            parser(nullptr) {
        }

        Record(std::string description, bool required, ParserFunction parser):
            description(description),
            required(required),
            parser(parser),
            supplied(false) {
        }

        void parse(std::string const & from) {
            parser(from);
            supplied = true;
        }
    };

    void parseArgument(std::string const & arg, std::string & name, std::string & value) {
        size_t i = 0;
        size_t nameEnd = std::string::npos;
        while (arg[i] != 0) {
            if (arg[i] == '=' and nameEnd == std::string::npos)
                nameEnd = i;
            ++i;
        }
        if (nameEnd == std::string::npos) {
            name = arg;
            value.clear();
        } else {
            name = arg.substr(0, nameEnd);
            value = arg.substr(nameEnd + 1, i - nameEnd - 1);
        }
    }

    std::unordered_map<std::string, Record> options_;

    Record defaultArgument_;
};


template<>
inline void Settings::ParseValue(std::string const & from, std::string & value) {
    value = from;
}

template<>
inline void Settings::ParseValue(std::string const & from, int & value) {
    try {
        value = std::stoi(from);
    } catch (...) {
        throw std::invalid_argument(STR("Value " << value << " cannot be converted to integer"));
    }
}

template<>
inline void Settings::ParseValue(std::string const & from, unsigned & value) {
    try {
        unsigned long result = std::stoul(from);
        if (result > UINT_MAX)
            throw 0;
        value = static_cast<unsigned>(result);
    } catch (...) {
        throw std::invalid_argument(STR("Value " << value << " cannot be converted to unsigned integer"));
    }
}

template<>
inline void Settings::ParseValue(std::string const & from, long & value) {
    try {
        value = std::stol(from);
    } catch (...) {
        throw std::invalid_argument(STR("Value " << value << " cannot be converted to unsigned long"));
    }
}

template<>
inline void Settings::ParseValue(std::string const & from, double & value) {
    try {
        value = std::stod(from);
    } catch (...) {
        throw std::invalid_argument(STR("Value " << value << " cannot be converted to double"));
    }
}

template<>
inline void Settings::ParseValue(std::string const & from, bool & value) {
    if (from.empty() || from == "1" || from == "T" || from == "Y" || from == "true" || from == "yes")
        value = true;
    else if (from == "0" || from == "F" || from == "N" || from == "false" || from == "no")
        value = true;
    else
        throw std::invalid_argument(STR("Value " << value << " cannot be converted to boolean"));
}

template<>
inline void Settings::ParseValue(std::string const & from, std::vector<std::string> & value) {
    value.push_back(from);
}



