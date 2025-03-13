#ifndef UTILS_HPP
#define UTILS_HPP

// C++ STL
#include <filesystem>
#include <chrono>
#include <future>

// Third Party
#include <fmt/color.h> // {fmt}
#include <fmt/xchar.h>
#include <curl/curl.h> // libcurl

#ifdef _WIN32
#define _IS_WINDOWS 1
#else
#define _IS_WINDOWS 0
#endif

constexpr static bool g_isWindows = _IS_WINDOWS;

#define FMT_ERROR_COLOR fmt::fg(fmt::color::red)|fmt::emphasis::bold
#define FMT_WARNING_COLOR fmt::fg(fmt::color::yellow)|fmt::emphasis::bold
#define FMT_SUCCESS_COLOR fmt::fg(fmt::color::green)|fmt::emphasis::bold

// Check if file exists
auto inline file_exists(const std::string& path) -> bool {
    return std::filesystem::exists({path});
}

// Check if file is directory
auto inline is_dir(const std::string& path) -> bool {
    return file_exists({path}) && std::filesystem::is_directory({path});
}

// Join paths (ex. "/dev", "urandom" => "/dev/urandom")
auto inline join_paths(const std::string& absolutePath, const std::string& relativePath) -> std::string {
    std::filesystem::path _abs{absolutePath};
    _abs /= {relativePath};
    return _abs.string();
}

// System specific functions
namespace os {
    // Start a new process
    int StartSubprocess(const std::string &programFile, std::vector<std::string> args, const std::string &cwd=".");
}

typedef std::future<std::pair<bool, std::string>> HttpResponse;

// CURL Write Function for API
static size_t curl_easy_writefn_str(void *data, const size_t chunkSize, const size_t numChunks, std::string *str) {
    const size_t totalSize = chunkSize * numChunks;
    str->append(static_cast<char*>(data), totalSize);
    return totalSize;
}

// Check if HttpResponse is ready
auto inline http_isReady(const HttpResponse& res) -> bool {
    return res.valid() && res.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

// Perform an HTTP GET request
auto inline http_get(const std::string& url) -> HttpResponse {
    return std::async(std::launch::async, [=]() -> std::pair<bool, std::string> {
            CURL* curl = curl_easy_init();
            if(curl == nullptr) throw std::runtime_error("Could not initialize CURL.");

            fmt::println("GET {}", url);
            // general configuration
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

            // receive data
            std::string data;
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_easy_writefn_str);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

            // send the request
            if(const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
                curl_easy_cleanup(curl);
                return std::make_pair<bool, std::string>(false, std::string(curl_easy_strerror(res)));
            }

            // request succeeded
            curl_easy_cleanup(curl);

            return std::make_pair<bool, std::string>(true, std::move(data));
        });
}

// Perform an automated blocking HTTP GET request
auto static make_http_request(const std::string& url) -> std::optional<std::string> {
    auto req = http_get(url);

    if (req.valid()) {
        req.wait();
    } else {
        fmt::print(stderr, FMT_ERROR_COLOR, "HTTP Request invalid\n");
        return std::nullopt;
    }

    if (auto [success, data] = req.get(); success)
        return data;
    else {
        fmt::print(stderr, FMT_ERROR_COLOR, "HTTP Request failed: {}\n", data);
        return std::nullopt;
    }
}

auto static make_dir_if_not_exists(const std::string& path) -> bool {
    if (file_exists(path)) {
        if (is_dir(path)) {
            return true;
        } else {
            fmt::print(stderr, FMT_ERROR_COLOR, "Path exists but is not a directory: {}\n", path);
            return false;
        }
    } else {
        if (std::filesystem::create_directories({path})) {
            return true;
        }
    }
    return false;
}

// TODO: Add Directory Constraints to all Lua functions
auto static directory_constraint_test(const std::string& projectPath, const std::string& givenPath) -> bool {
    const auto absoluteProjectPath = std::filesystem::absolute(std::filesystem::canonical({projectPath}));
    const auto absoluteGivenPath = std::filesystem::absolute(std::filesystem::canonical({givenPath}));

    return absoluteGivenPath.string().starts_with(absoluteProjectPath.string());
}

// Find all files with extension in a directory
auto static find_files_with_extensions(const std::string& path, std::vector<std::string>&& extensions) {
    std::vector<std::string> files{};
    if (file_exists(path) && is_dir(path)) {
        for (const auto& directory_entry : std::filesystem::recursive_directory_iterator(path)) {
            if (directory_entry.is_regular_file()) {
                if (auto ext = directory_entry.path().extension(); std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                    files.push_back(directory_entry.path().string());
                }
            }
        }
    }
    return files;
}

// Get all directories in PATH environment variable
auto static get_path_dirs() -> std::vector<std::string> {
    const auto* path = std::getenv("PATH");

    constexpr char delim = (g_isWindows ? ';' : ':');
    std::string tmp;
    std::stringstream ss(path);
    std::vector<std::string> dirs;

    while(std::getline(ss, tmp, delim)){
        dirs.push_back(tmp);
    }

    return dirs;
}

// Find an executable in PATH
auto static find_exe(std::string&& name) -> std::optional<std::string> {
    const std::string executable = name + std::string(g_isWindows ? ".exe" : "");

    // Check for an existing ninja executable
    for (auto dirs = get_path_dirs(); const auto& dir : dirs) {
        if (auto path = join_paths(dir, executable); file_exists(path)) {
            return path;
        }
    }

    return std::nullopt;
}

#endif //UTILS_HPP
