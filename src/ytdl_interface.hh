#include <string>
#include <vector>
#include <memory>
#include <cstdio>
#include <iostream>
#include <unistd.h>

namespace ytdl
{
    using namespace std::string_literals;

    namespace detail
    {
        std::vector<std::string> exec(std::string const &cmd)
        {
            auto pipe = popen(cmd.c_str(), "r");
            if (!pipe)
                throw std::runtime_error("Could not run "s + cmd);

            std::vector<std::string> result;
            thread_local std::array<char, 2048> buffer;

            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
                result.emplace_back(buffer.data());

            pclose(pipe);
            return result;
        }
    }

    std::vector<std::string> extractAudioURLs(std::string const &video_or_playlist_url)
    {
        auto result = detail::exec("youtube-dl -g --extract-audio \""s + video_or_playlist_url + "\"");
        for (auto &line : result)
            if (line.back() == '\n')
                line.erase(line.end() - 1);
        return result;
    }
}