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
static bool g_allowShell = true;

// Locations of executables
// TODO: Pass these as accessible commands to Lua
// NOTE: These are NOT required for Ninja. As far as that goes, they just need to be in PATH.
struct Tools {
    std::optional<std::string> ninja{}; // Ninja Build System,
    std::optional<std::string> cmake{}; // CMake Build System Generator
    std::optional<std::string> make{}; // Make Build System
    std::optional<std::string> gcc{}; // GCC compiler
    std::optional<std::string> gpp{}; // GCC C++ compiler
    std::optional<std::string> clang{}; // Clang C compiler
    std::optional<std::string> clangpp{}; // Clang C++ compiler
    // Linux
    std::optional<std::string> ar{}; // Linux Archive (.a) Packager
    // Windows
    std::optional<std::string> msvc_cl{}; // Microsoft Visual C++ Compiler
    std::optional<std::string> msvc_link{}; // Microsoft Visual C++ Linker
    std::optional<std::string> msvc_lib{}; // Microsoft Visual C++ Library Manager
    std::optional<std::string> ms_build{}; // Microsoft Build System (.sln)
    std::optional<std::string> ms_nmake{}; // Microsoft NMake Build System
    // std::optional<std::string> msvc_rc{}; // Microsoft Resource Compiler
    // std::optional<std::string> msvc_mt{}; // Microsoft Manifest Tool
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

-- Project settings
local projectName = "myProject"
local version = "1.0.0"

-- Project files
local sourceFiles = find_source_files(projectDir .. "/src")
local includeDirs = find_header_files(projectDir .. "/include")
local cFlags = ""
local ldFlags = ""

-- Debug config
function debug()
    add_project(
        projectName,                  -- Name
        "1.0.0",                      -- Version
        sourceFiles,                  -- Source files (Lua array -> std::vector)
        includeDirs,                  -- Include directories (Lua array -> std::vector)
        {},                           -- Library directories (Lua array -> std::vector)
        {},                           -- Dependencies (Lua array -> std::vector)
        cFlags, cFlags, ldFlags,      -- (C,CXX,LD) Flags
        outputDir,                    -- Output directory
        "executable",                 -- Build type
        "default",                    -- Compiler
        "", "", ""                    -- (C,CXX,LD) Export Flags
    )

    build()
end

if config == "debug" then
    debug()
else
    print('To do: add more configs')
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

    static bool SystemCommand(std::string command, const std::vector<std::string> args) {
        if (!g_allowShell) {
            fmt::print(FMT_ERROR_COLOR, "SHELL COMMANDS ARE DISABLED. Use --allow-shell to enable.\n");
            return false;
        }

        if (command.find("/") == std::string::npos && command.find("\\") == std::string::npos) {
            std::string _command = command;
            command = find_exe(std::move(_command)).value_or(command);
        }

        if (!g_autoYes) {
            fmt::print(FMT_WARNING_COLOR, "WARNING: SHELL COMMANDS COULD COMPROMISE YOUR SYSTEM.\n");
            fmt::println("SH: {} {}", command, fmt::join(args, " "));
            fmt::print("Continue with command? (Y/n) ");
            if (const char in = std::getchar(); in != 'y' && in != 'Y') {
                fmt::print(FMT_ERROR_COLOR, "Command aborted.\n");
                return false;
            }
        } else fmt::println("SH: {} {}", command, fmt::join(args, " "));

        return os::StartSubprocess(command, args) == 0;
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

    // LLUA:build
    static void BuildProjects() {
        fmt::println("Building project(s)...");
        g_Generator.generate();
        g_Generator.reset();
        os::StartSubprocess(g_tools.ninja.value(), {});
    }

    // LUA: add_dependency
    static void AddDependency(
        const std::string& dependencyName,
        const std::string& version,
        std::vector<std::string> libraryPaths,
        std::vector<std::string> includeDirs,
        const std::string& cFlags,
        const std::string& cxxFlags,
        const std::string& ldFlags
        ) {
        for (const auto& path : libraryPaths) {
            if (!file_exists(path)) {
                fmt::print(stderr, FMT_ERROR_COLOR, "Invalid library path: {}\n", path);
                throw std::runtime_error("Invalid library path");
            }
        }
        Dependency _dependency = {
            .dependencyName = dependencyName,
            .dependencyVersion = version,
            .libraryPaths = libraryPaths,
            .includeDirs = includeDirs,
            .cFlags = cFlags,
            .cxxFlags = cxxFlags,
            .ldFlags = ldFlags
        };

        g_Generator.add(_dependency);
    }

    // LUA: add_project
    static void AddProject(
        const sol::this_state lua_state,
        const std::string& projectName,
        const std::string& version,
        const std::vector<std::string> sourceFiles,
        const std::vector<std::string> includeDirs,
        const std::vector<std::string> libDirs,
        const std::vector<std::string> dependencies,
        const std::string& cFlags,
        const std::string& cxxFlags,
        const std::string& ldFlags,
        const std::string& outputDir,
        const std::string& buildType,
        const std::string& compiler,
        const std::string& cFlagsOut,
        const std::string& cxxFlagsOut,
        const std::string& ldFlagsOut
        ) {
        sol::state_view lua(lua_state);

        if (!make_dir_if_not_exists(outputDir)) {
            throw std::runtime_error("Failed to create output directory");
        }

        Project _project = {
            .projectName = projectName,
            .version = version,
            .sourceFiles = sourceFiles,
            .includeDirs = includeDirs,
            .libDirs = libDirs,
            .dependencies = dependencies,
            .cFlags = cFlags,
            .cxxFlags = cxxFlags,
            .ldFlags = ldFlags,
            .outputPath = std::string(), // default
            .buildType = ProjectBuildType::BUILD_NO_LINK, // default
            .compiler = CompilerType::GCC, // unset
            .cFlagsOut = cFlagsOut,
            .cxxFlagsOut = cxxFlagsOut,
            .ldFlagsOut = ldFlagsOut
        };

        if (buildType == "executable") {
            _project.buildType = ProjectBuildType::EXECUTABLE;
        } else if (buildType == "static") {
            _project.buildType = ProjectBuildType::STATIC_LIBRARY;
        } else if (buildType == "shared") {
            _project.buildType = ProjectBuildType::SHARED_LIBRARY;
        } else if (buildType != "object") {
            fmt::println("Unrecognized project type. Defaulting to 'object' option.");
        }

        _project.outputPath = _project.buildType == ProjectBuildType::BUILD_NO_LINK ? outputDir : join_paths(outputDir, NinjaGenerator::ProjectNameToFileName(_project.projectName, _project.buildType));

        if (compiler == "clang") {
            _project.compiler = CompilerType::CLANG;
        } else if (compiler == "msvc") {
            _project.compiler = CompilerType::MSVC;
        } else if (compiler != "gcc") {
            if(g_isWindows) {
                _project.compiler = CompilerType::MSVC;
                if (compiler != "default")
                fmt::println("Unrecognized compiler. Defaulting to MSVC.");
            } else {
                if (compiler != "default")
                fmt::println("Unrecognized compiler. Defaulting to GCC.");
            }
        }

        g_Generator.add(_project);

        _project.print();
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
        m_lua.set_function("add_dependency", &LuaInstance::AddDependency);
        m_lua.set_function("add_project", &LuaInstance::AddProject);
        m_lua.set_function("build", &LuaInstance::BuildProjects);
        m_lua.set_function("find_source_files", &LuaInstance::FindSourceFiles);
        m_lua.set_function("find_module_files", &LuaInstance::FindModuleFiles);
        m_lua.set_function("find_header_files", &LuaInstance::FindHeaderFiles);
        m_lua.set_function("cmake", &LuaInstance::BuildCMakeProject);
        m_lua.set_function("http_get", &LuaInstance::HttpGet);
        m_lua.set_function("download", &LuaInstance::download);
        m_lua.set_function("read_file", &LuaInstance::ReadFile);
        m_lua.set_function("run_command", &LuaInstance::SystemCommand);
    }

    void run_script(const std::string& scriptPath) {
        if (const auto result = m_lua.safe_script_file(scriptPath); !result.valid()) {
            const sol::error err = result;
            fmt::print(stderr, FMT_ERROR_COLOR, "Error in Lua script: {}\n", err.what());
            std::exit(EXIT_FAILURE);
        }
    }

private:
    sol::state m_lua;
};

// cpkg --script
auto run_lua_script(const std::string& luaScriptPath) {
    if(!file_exists(luaScriptPath)) throw std::runtime_error("File does not exist.");
    LuaInstance lua;
    lua.get().set("projectDir", sol::nil);
    lua.get().set("outputDir", sol::nil);
    lua.get().set("config", sol::nil);
    if (g_isWindows) {
        lua.get().set("platform", "windows");
    } else {
        lua.get().set("platform", "unix");
    }
    lua.run_script(luaScriptPath);
}

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
        ("clean", "Clean the build directory")
        ("b,build", "Build the project")
        ("script", "Run a Lua script", cxxopts::value<std::string>())
        ("d,dir", "Project directory", cxxopts::value<std::string>()->default_value("./"))
        ("c,config", "Project configuration to use", cxxopts::value<std::string>()->default_value("debug"))
        ("o,output", "Output directory", cxxopts::value<std::string>()->default_value("build/"))
        ("y,yes", "Answer 'yes' to all warnings (Be cautious!)", cxxopts::value<bool>()->implicit_value("true")->default_value("false"))
        ("allow-shell", "Allow system commands. Auto-enabled unless -y is passed.", cxxopts::value<bool>()->implicit_value("true"))
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
        if (options_results.count("allow-shell") == 0U) {
            g_allowShell = false;
        }
        fmt::print(FMT_WARNING_COLOR, "Auto-yes enabled. Warnings will not require user intervention.\n\n");
    }

    if (options_results.count("clean")) {
        os::StartSubprocess(g_tools.ninja.value(), {"-t", "clean"});
        return EXIT_SUCCESS;
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
        fmt::println("Process finished.");
        return EXIT_SUCCESS;
    }

    // Run a script
    if(options_results.count("script")) {
        std::string scriptPath = options_results["script"].as<std::string>();
        fmt::println("Running script.");
        run_lua_script(scriptPath);
        fmt::println("Process finished.");
        return EXIT_SUCCESS;
    }

    // Test connection to server if no commands specified
    ([]() -> void {
        fmt::println("Testing connection to community server...");
        if (!make_http_request("https://getcpkg.net").has_value()) {
            fmt::print(stderr, FMT_WARNING_COLOR, "Failed to connect to server. Please check your internet connection.\n\n");
        } else {
            fmt::print(FMT_SUCCESS_COLOR, "Server is online!\n\n");
        }
    })();

    // Help
    fmt::println("{}", options.help());
    return EXIT_SUCCESS;
}

/* END OF FILE */