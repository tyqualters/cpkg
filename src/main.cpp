// C STL
#include <cstdio>
#include <cstdint>
#include <cstdlib>

// C++ STL
#include <fstream>
#include <filesystem>
#include <vector>
#include <optional>
#include <chrono>
#include <future>

// Third Party
#include <fmt/color.h> // {fmt}
#include <sol/sol.hpp> // sol2/Lua
#include <cxxopts.hpp> // cxxopts

// First Party
#include "utils.hpp"
#include "ninja_generator.hpp"

#define CPKG_VERSION "1.0"

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

NinjaGenerator g_Generator;

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

    // LUA:dir_exists
    static bool DirectoryExists(const std::string& path) {
        return is_dir(path);
    }

    // LUA:is_dir
    static bool IsDir(const std::string& path) {
        return is_dir(path);
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
        m_lua.set_function("dir_exists", &LuaInstance::DirectoryExists);
        m_lua.set_function("is_dir", &LuaInstance::IsDir);
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
    if (g_isWindows) {
        lua.get().set("platform", "windows");
    } else {
        lua.get().set("platform", "unix");
    }

    lua.run_script(buildScriptPath);
}

/* MAIN FUNCTION */

int main(int argc, char* argv[]) {
    fmt::println("cpkg version " CPKG_VERSION " \u00A9 Ty Qualters 2025");
    fmt::println("Bundled with Lua " LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "\n");

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
        os::StartSubprocess(g_tools.ninja.value(), {});
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

/* END OF FILE */