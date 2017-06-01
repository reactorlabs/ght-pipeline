#pragma once

#include <mutex>
#include <vector>
#include <string>


class Github {
public:

    static std::vector<std::string> ApiTokens;

    static std::string NextApiToken() {
        static std::mutex m;
        static unsigned i = 0;
        std::lock_guard<std::mutex> g(m);
        unsigned j = i++;
        if (i == ApiTokens.size())
            i = 0;
        return ApiTokens[j];
    }

};
