// C STL
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

// C++ STL
#include <fstream>
#include <filesystem>
#include <vector>
#include <optional>
#include <chrono>
#include <future>
#include <unordered_map>
#include <variant>

// Third Party
#include <fmt/color.h> // {fmt}
#include <fmt/xchar.h>
#include <sol/sol.hpp> // sol2/Lua
#include <cxxopts.hpp> // cxxopts
#include <curl/curl.h> // libcurl

// First Party
#include "ninja_syntax.hpp"

#define CPKG_VERSION "1.0"

#ifdef _WIN32
#define _IS_WINDOWS 1
#else
#define _IS_WINDOWS 0
#endif

#define FMT_ERROR_COLOR fmt::fg(fmt::color::red)|fmt::emphasis::bold
#define FMT_WARNING_COLOR fmt::fg(fmt::color::yellow)|fmt::emphasis::bold
#define FMT_SUCCESS_COLOR fmt::fg(fmt::color::green)|fmt::emphasis::bold

constexpr bool g_isWindows = _IS_WINDOWS;

static bool g_autoYes = false;

// Locations of executables
struct Tools {
    std::optional<std::string> ninja{};
    std::optional<std::string> cmake{};
    std::optional<std::string> make{};
    std::optional<std::string> gcc{};
    std::optional<std::string> clang{};
    std::optional<std::string> msvc{};
} static g_tools;

/**
 * Feature list required for a build script:
 *  - Select compiler (GCC, MSVC, Clang)
 *  - Add compiler option/flag (formatted for compiler)
 *  - Add linker option/flag (formatted for compiler)
 *  - Find files (RegEx)
 *  - Build as EXE, Static, Shared
 *  - Add a dependency (auto)
 *  - Add another project (manual) - Find/Call executables like CMake, Make, Meson, Ninja, etc. (Limited for security)
 *  - File/Directory exists(?) T/F
 *  - Download project from URL - SCAN FILE and WARN user of URLs first
 *  - Unzip / Untar
 *  - Run a command in shell (CMD) / SCAN FILE and WARN user first
 *
 *  - Download community Lua file that will automate all this for you :)
 */
const char* const lua_init_script = R"lua(
-- Quick Reference:
-- > projectDir: The directory of the project file
-- > outputDir: The directory to output the build files to
-- > config: The configuration to build for

local projectName = "default"
local version = "1.0.0"

local sourceFiles = find_source_files(projectDir .. "/src")
local includeDirs = {projectDir .. "/include"}
local compilerFlags = ""
local linkerFlags = ""

function debug()
    print('Building ' .. projectName .. ' (Debug)')
    build(projectName, version, sourceFiles, headerFiles, compilerFlags, linkerFlags, outputDir)
end

function release()
    -- TODO: Add optimization
    print('Building ' .. projectName .. ' (Release)')
    build(projectName, version, sourceFiles, headerFiles, compilerFlags, linkerFlags, outputDir)
end

if config == "debug" then
    debug()
elseif config == "release" then
    release()
else
    error("Unknown config")
end
)lua";

// Check if file exists
auto inline file_exists(const std::string& path) -> bool {
    return std::filesystem::exists({path});
}

// Check if file is directory
auto inline is_dir(const std::string& path) -> bool {
    return std::filesystem::is_directory({path});
}

// Join paths (ex. "/dev", "urandom" => "/dev/urandom")
auto join_paths(const std::string& absolutePath, const std::string& relativePath) -> std::string {
    std::filesystem::path _abs{absolutePath};
    _abs /= {relativePath};
    return _abs.string();
}

// System specific functions
namespace os {
#if _IS_WINDOWS == 1
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
// #include <unistd.h>
#include <spawn.h>
#include <wait.h>
#endif

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
auto http_get(const std::string& url) -> HttpResponse {
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
auto make_http_request(const std::string& url) -> std::optional<std::string> {
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

auto make_dir_if_not_exists(const std::string& path) -> bool {
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
auto directory_constraint_test(const std::string& projectPath, const std::string& givenPath) -> bool {
    const auto absoluteProjectPath = std::filesystem::absolute(std::filesystem::canonical({projectPath}));
    const auto absoluteGivenPath = std::filesystem::absolute(std::filesystem::canonical({givenPath}));

    return absoluteGivenPath.string().starts_with(absoluteProjectPath.string());
}

// Find all files with extension in a directory
auto find_files_with_extensions(const std::string& path, std::vector<std::string>&& extensions) {
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

// A BuildConfiguration option that NinjaGenerator will use
enum class ProjectBuildType {
    EXECUTABLE,
    STATIC_LIBRARY,
    SHARED_LIBRARY,
    BUILD_NO_LINK // compile but do not call linker
};
struct Project {
    std::string projectName;
    std::vector<std::string> sourceFiles;
    std::vector<std::string> includeDirs;
    std::unordered_map<std::string, std::string> dependencies;
    std::string compilerFlags;
    std::string linkerFlags;
    std::string outputPath;
    ProjectBuildType buildType;
};

class NinjaGenerator {
public:
    NinjaGenerator() = default;
    ~NinjaGenerator() = default;

    void add(const Project& project) {
        if (m_projectNames.contains(project.projectName)) {
            throw std::runtime_error("Project name already exists.");
        }
        m_contents << "rule cc\n";
        m_contents << "  command = " << g_tools.cmake.value() << " -E env CC=$CC CXX=$CXX CXXFLAGS=\"$CXXFLAGS $compilerFlags\" LDFLAGS=\"$LDFLAGS $linkerFlags\" cmake --build $buildDir --target $target -- -j $jobs\n";
        m_contents << "  description = Compiling $target\n";
        m_contents << "  depfile = $out.d\n";
        m_contents << "  deps = gcc\n";
        m_contents << "\n";
        m_contents << "rule link\n";
        m_contents << "  command = " << g_tools.cmake.value() << " -E env CC=$CC CXX=$CXX CXXFLAGS=\"$CXXFLAGS $compilerFlags\" LDFLAGS=\"$LDFLAGS $linkerFlags\" cmake --build $buildDir --target $target -- -j $jobs\n";
        m_contents << "  description = Linking $target\n";
    }

    void reset() {
        m_contents.clear();
        m_contents.str("");
    }

    // Generate build.ninja
    void generate() const {
        std::ofstream ofs("build.ninja");
        if (!ofs.is_open()) {
            throw std::runtime_error("Failed to open build.ninja");
        }
        ofs << m_contents.str();
        ofs.close();
    }

    // Call Ninja
    static void build() {
        os::StartSubprocess(g_tools.ninja.value(), {});
    }

    std::stringstream m_contents;
    std::unordered_set<std::string> m_projectNames;
} g_Generator;

class LuaInstance {
public:
    LuaInstance() {
        m_lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::utf8);
        bind_functions();
    }

    auto& get() {
        return m_lua;
    }

    // LUA:http_get
    static sol::optional<std::string> HttpGet(const std::string& url) {
        if (auto result = make_http_request(url); result.has_value()) {
            return result.value();
        } else {
            return sol::nullopt;
        }
    }

    // LUA:download
    static bool download(const std::string& url, const std::string& path) {
        if (const auto result = make_http_request(url); result.has_value()) {
            const auto& data = result.value();
            std::ofstream ofs(path, std::ios::trunc);
            if (!ofs.is_open()) {
                fmt::print(stderr, FMT_ERROR_COLOR, "Download failed: File write error\n");
                return false;
            }
            ofs << data;
            ofs.close();
            return true;
        }
        fmt::print(stderr, FMT_ERROR_COLOR, "Download failed: HTTP Request error\n");
        return false;
    }

    // LUA:file_exists
    static bool FileExists(const std::string& path) {
        return file_exists(path);
    }

    // LUA:build
    // TODO: build, build_lib_shared, build_lib_static
    static void BuildProject(const sol::this_state lua_state, std::string projectName, std::string version, std::vector<std::string> sourceFiles, std::vector<std::string> headerFiles, std::string compilerFlags, std::string linkerFlags, std::string outputDir) {
        sol::state_view lua(lua_state);

        if (!make_dir_if_not_exists(outputDir)) {
            throw std::runtime_error("Failed to create output directory");
        }

        g_Generator.add({
            .projectName = projectName,
            .sourceFiles = sourceFiles,
            .includeDirs = headerFiles,
            .dependencies = {},
            .compilerFlags = compilerFlags,
            .linkerFlags = linkerFlags,
            .outputPath = join_paths(outputDir, projectName),
            .buildType = ProjectBuildType::BUILD_NO_LINK,
        });

        fmt::println("Adding project {} (version {})...", projectName, version);
        for (const auto& file : sourceFiles) {
            fmt::println("src - {}", file);
        }
        for (const auto& file : headerFiles) {
            fmt::println("hdr - {}", file);
        }
        fmt::println("Compiler flags: {}", compilerFlags);
        fmt::println("Linker flags: {}", linkerFlags);
        fmt::println("Output directory: {}", outputDir);
    }

    // LUA:find_source_files
    static std::vector<std::string> FindSourceFiles(const std::string& path) {
        return find_files_with_extensions(path, {".cpp", ".cxx", ".cc", ".c"});
    }

    // LUA:find_module_files
    static std::vector<std::string> FindModuleFiles(const std::string& path) {
        return find_files_with_extensions(path, {".ixx", ".mxx", ".cppm", ".cxxm", ".cmxx"});
    }

    // LUA:find_header_files
    static std::vector<std::string> FindHeaderFiles(const std::string& path) {
        return find_files_with_extensions(path, {".hpp", ".hxx", ".hh", ".hh"});
    }

    // LUA:cmake
    static void BuildCMakeProject(const std::string& projectDir) {
        os::StartSubprocess(g_tools.cmake.value(), {"-B", projectDir + "/build", "-S", projectDir});
    }

    // static std::vector<std::string> LuaEmptyArrayFunction() {
    //     return {};
    // }

    //LUA:read_file
    static sol::optional<std::string> ReadFile(const std::string& path) {
        std::ifstream ifs(path);
        if (!ifs.is_open()) {
            fmt::print(stderr, FMT_ERROR_COLOR, "Failed to open file: {}\n", path);
            return sol::nullopt;
        }
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        return buffer.str();
    }

    // TODO: Integrity Check files
    // static bool HashMatch(const std::string& path, std::string hash) {
    //     if (file_exists(path)) {
    //
    //     } else {
    //         fmt::print(stderr, FMT_ERROR_COLOR, "Cannot verify hash. File does not exist: {}\n", path);
    //         return false;
    //     }
    // }

    // Bind C++ functions to Lua
    void bind_functions() {
        m_lua.set_function("file_exists", &LuaInstance::FileExists);
        m_lua.set_function("build", &LuaInstance::BuildProject);
        m_lua.set_function("find_source_files", &LuaInstance::FindSourceFiles);
        m_lua.set_function("find_module_files", &LuaInstance::FindModuleFiles);
        m_lua.set_function("find_header_files", &LuaInstance::FindHeaderFiles);
        m_lua.set_function("cmake", &LuaInstance::BuildCMakeProject);
        m_lua.set_function("http_get", &LuaInstance::HttpGet);
        m_lua.set_function("download", &LuaInstance::download);
        m_lua.set_function("read_file", &LuaInstance::ReadFile);
    }

    void run_script(const std::string& scriptPath) {
        if (const auto result = m_lua.safe_script_file(scriptPath); !result.valid()) {
            fmt::print(stderr, FMT_ERROR_COLOR, "Error in Lua script: {}\n", static_cast<sol::error>(result).what());
            std::exit(EXIT_FAILURE);
        }
    }

private:
    sol::state m_lua;
};

// cpkg --build
auto run_build_script(const std::string& projectPath, const std::string& buildPath, const std::string& config) {
    auto buildScriptPath = join_paths(projectPath, "cpkg.lua");
    if (!file_exists(buildScriptPath) || !is_dir(projectPath)) {
        fmt::print(stderr, FMT_ERROR_COLOR, "Project file located at {} does not exist.\n", buildScriptPath);
        fmt::println("Please run `cpkg --init` to generate a project file.");
        std::exit(EXIT_FAILURE);
    }

    if (file_exists(buildPath)) {
        if (!is_dir(buildPath)) {
            fmt::print(stderr, FMT_ERROR_COLOR, "Build directory located at {} does not exist.\n", buildPath);
            fmt::println("File with same name as output directory. Please fix.");
            std::exit(EXIT_FAILURE);
        }
    } else {
        if (!std::filesystem::create_directory(buildPath)) {
            fmt::print(stderr, FMT_ERROR_COLOR, "Failed to create build directory.\n", buildPath);
            fmt::println("Please check permissions.");
            std::exit(EXIT_FAILURE);
        }
    }

    LuaInstance lua;
    lua.get().set("projectDir", projectPath);
    lua.get().set("outputDir", join_paths(projectPath, buildPath));
    lua.get().set("config", "debug");

    lua.run_script(buildScriptPath);
}

// Get all directories in PATH environment variable
auto inline get_path_dirs() -> std::vector<std::string> {
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
auto find_exe(std::string&& name) -> std::optional<std::string> {
    const std::string executable = name + std::string(g_isWindows ? ".exe" : "");

    // Check for an existing ninja executable
    for (auto dirs = get_path_dirs(); const auto& dir : dirs) {
        if (auto path = join_paths(dir, executable); file_exists(path)) {
            return path;
        }
    }

    return std::nullopt;
}

/* MAIN FUNCTION */

int main(int argc, char* argv[]) {
    fmt::println("cpkg version " CPKG_VERSION " \u00A9 Ty Qualters 2025");
    fmt::println("Bundled with Lua " LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "\n");

    NinjaWriter writer;
    writer.variable("cc", "gcc");
    writer.rule("cc", "$cc $cflags -c -o $out $in");
    writer.rule("ld", "$cc -o $out $in $ldflags");
    writer.build("main", "ld", std::vector<std::string>{"main.o", "lib.o"});
    writer.build("main.o", "cc", "main.c");
    writer.build("lib.o", "cc", "lib.c", {"lib.h"});

    fmt::println("{}", writer.string());

    std::cin.get();
    return 0;
    // os::StartSubprocess("/bin/ls", {"-l"}, ".");

    // CURL: RAII init and clean up
    struct _curl {
        _curl() {
            curl_global_init(CURL_GLOBAL_ALL);
        }
        ~_curl() {
            curl_global_cleanup();
        }
    } const _curl;

    // Search for tools (populate g_tools)
    if (auto cmake = find_exe("cmake"); cmake.has_value()) {
        fmt::println("Found CMake at {}", cmake.value());
        g_tools.cmake = cmake.value();
    } else {
        fmt::println("Did not find CMake. If installed, add to your PATH.");
    }

    if (auto ninja = find_exe("ninja"); ninja.has_value()) {
        fmt::println("Found Ninja at {}\n", ninja.value());
        g_tools.ninja = ninja.value();
    } else {
        // Ninja is required
        fmt::print(stderr, FMT_ERROR_COLOR, "Did not find Ninja. Try installing it and adding it to your PATH.\n");
        return EXIT_FAILURE;
    }

    // Parse command line arguments
    cxxopts::Options options("cpkg", "cpkg is a project management tool for C/C++");

    options.add_options()
        ("i,init", "Generate a cpkg.lua file")
        ("b,build", "Build the project")
        ("d,dir", "Project directory", cxxopts::value<std::string>()->default_value("./"))
        ("c,config", "Project configuration to use", cxxopts::value<std::string>()->default_value("debug"))
        ("CC", "Which C compiler to use", cxxopts::value<std::string>()->default_value("gcc")) // TODO
        ("CXX", "Which C++ compiler to use", cxxopts::value<std::string>()->default_value("g++")) // TODO
        ("CFLAGS", "Flags to pass to the C compiler", cxxopts::value<std::string>()) // TODO
        ("CXXFLAGS", "Flags to pass to the C++ compiler", cxxopts::value<std::string>()) // TODO
        ("LDFLAGS", "Flags to pass to the linker", cxxopts::value<std::string>()) // TODO
        ("o,output", "Output directory", cxxopts::value<std::string>()->default_value("build/")) // TODO
        ("y,yes", "Answer 'yes' to all warnings (Be cautious!)", cxxopts::value<bool>()->implicit_value("true")->default_value("false"))
        ("h,help", "Print help");

    // options.parse_positional({"add", "dependency-name"});
    // options.allow_unrecognised_options();

    auto options_results = options.parse(argc, argv);

    // Help
    if (options_results.count("h")) {
        fmt::println("{}", options.help());
        return EXIT_SUCCESS;
    }

    // Auto-yes
    if (options_results.count("y")) {
        g_autoYes = true;
        fmt::print(FMT_WARNING_COLOR, "Auto-yes enabled. Warnings will not require user intervention.\n\n");
    }

    // Init new cpkg.lua file
    if (options_results.count("i")) {
        if (file_exists("cpkg.lua")) {
            fmt::print(stderr, FMT_ERROR_COLOR, "cpkg.lua already exists in the current directory.\n");
            return EXIT_FAILURE;
        }
        std::ofstream ofs("cpkg.lua", std::ios::trunc);
        ofs << lua_init_script;
        ofs.close();
        fmt::println("cpkg.lua generated successfully.");
        return EXIT_SUCCESS;
    }

    // Build a project
    if (options_results.count("b")) {
        std::string projectPath = options_results["dir"].as<std::string>();
        std::string buildPath = options_results["output"].as<std::string>();
        std::string config = options_results["config"].as<std::string>();
        run_build_script(projectPath, buildPath, config);
        fmt::println("Generating build.ninja.");
        g_Generator.generate();
        fmt::println("Building project.");
        g_Generator.build();
        fmt::println("Process finished.");
        return EXIT_SUCCESS;
    }

    // Test connection to server if no commands specified
    []() static -> void {
        fmt::println("Testing connection to community server...");
        if (!make_http_request("https://getcpkg.net").has_value()) {
            fmt::print(stderr, FMT_WARNING_COLOR, "Failed to connect to server. Please check your internet connection.\n\n");
        } else {
            fmt::print(FMT_SUCCESS_COLOR, "Server is online!\n\n");
        }
    }();

    // Help
    fmt::println("{}", options.help());
    return EXIT_SUCCESS;
}

/* SYSTEM SPECIFIC FEATURES */
int os::StartSubprocess(const std::string &programFile, std::vector<std::string> args, const std::string &cwd) {
    if (!file_exists(programFile)) {
        throw std::runtime_error("Program not found.");
    }

    // Use this as first argument in argv
    auto programName = std::filesystem::path{programFile}.filename().string();

#if _IS_WINDOWS == 1
    // TODO: Test Windows

    // Convert arguments to char* format
    std::stringstream ss;
    for (size_t i = 0; i < args.size(); ++i) {
        ss << args[i];
        if (i != args.size() - 1) {
            ss << " ";
        }
    }

    std::string _argv = ss.str();
    char* argv = new char[_argv.size() + 1];
    std::strcopy(argv, _argv.c_str());

    // Define process information
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    // Start the process
    if (CreateProcessA(programFile.c_str(), argv, nullptr, nullptr, FALSE, 0, nullptr, cwd.c_str(), &si, &pi) != TRUE) {
        throw std::system_error(GetLastError(), std::system_category(), "Failed to start process at " + programFile);
    }

    // Wait for process to end
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Get exit code
    DWORD exitCode;
    if (GetExitCodeProcess(procInfo.hProcess, &exitCode)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return reinterpret_cast<int>(exitCode);
    } else {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        fmt::print(stderr, FMT_ERROR_COLOR, "Failed to get exit code of process.\n");
        return EXIT_FAILURE;
    }
#else
    // Convert arguments to char* const[] format
    std::vector<char*> argv;
    argv.reserve(args.size() + 2U);
    argv.push_back(const_cast<char*>(programName.c_str()));
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    // Set attributes
    posix_spawn_file_actions_t file_actions;
    posix_spawnattr_t attr;
    if (const int ret = posix_spawn_file_actions_init(&file_actions); ret != 0) {
        throw std::system_error(ret, std::system_category(), "Failed to initialize spawn file actions.");
    }
    if (const int ret = posix_spawnattr_init(&attr); ret != 0) {
        throw std::system_error(ret, std::system_category(), "Failed to initialize spawn attributes.");
    }

    // Change current working directory
    if (cwd != ".")
        if (const int ret = posix_spawn_file_actions_addchdir_np(&file_actions, cwd.c_str()); ret != 0) {
            throw std::system_error(ret, std::system_category(), "Failed to change spawn current working directory.");
        }

    // Start the process
    fmt::println("CMD {} {}", programFile.c_str(), fmt::join(args, " "));
    pid_t pid;
    if (const int ret = posix_spawn(&pid, programFile.c_str(), &file_actions, &attr, argv.data(), environ); ret != 0) {
        throw std::system_error(ret, std::system_category(), "Failed to start process at " + programFile);
    }

    // Wait for process to end
    int status;
    waitpid(pid, &status, 0);

    // Clean up
    posix_spawn_file_actions_destroy(&file_actions);
    posix_spawnattr_destroy(&attr);

    // Return the process exit code
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        fmt::print(stderr, FMT_ERROR_COLOR, "{} exited abnormally.\n", programName);
        return EXIT_FAILURE;
    }
#endif
}

/* END OF FILE */