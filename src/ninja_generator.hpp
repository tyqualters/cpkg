#ifndef NINJA_GENERATOR_HPP
#define NINJA_GENERATOR_HPP

// C++ STL
#include <fstream>
#include <unordered_set>

// First Party
#include "ninja_syntax.hpp"

/**
 *  TODO: Check if the Ninja file needs to be regenerated before regenerating it.
 *
 *  Build Process for Ninja File Generation
 *
 *  1. **Project Creation (Lua Initialization)**
 *      - Lua script generates the project structure.
 *      - Configures source files, include directories, and target output paths.
 *
 *  2. **Select and Configure Compiler**
 *      - Choose the appropriate compiler based on the environment (e.g., GCC, Clang, MSVC).
 *      - Ensure the correct toolchain is used based on project settings.
 *      - Set `C` (C compiler) and `CC` (C++ compiler) flags dynamically, possibly by environment or configuration.
 *
 *  3. **Configure Project Flags**
 *      - Set `CFLAGS`, `CXXFLAGS`, and `LDFLAGS` for the project.
 *      - Configure additional flags based on platform-specific requirements.
 *      - Validate flags to ensure compatibility with the selected compiler.
 *
 *  4. **Add Dependency Flags**
 *      - Parse external dependencies (e.g., libraries, other projects).
 *      - Append their `CFLAGS`, `CXXFLAGS`, and `LDFLAGS` to the project’s flags.
 *      - Include relevant paths (e.g., `-I` for headers, `-L` for library directories).
 *
 *  5. **Dependency Tracking in Ninja**
 *      - Specify dependencies in Ninja syntax (e.g., `projectName : dependencyName`).
 *      - Ensure that dependencies are correctly defined and versioned for proper incremental builds.
 *      - Define dependency graph so Ninja knows the correct build order.
 *
 *  6. **Building the Project**
 *      - For C/C++ projects, generate build rules for compilation (e.g., `g++ -c source.c`).
 *      - Create individual compilation steps for each source file.
 *      - Enable optimizations for faster builds (e.g., `-O2` for GCC/Clang).
 *      - Define custom build steps if needed (e.g., code generation, preprocessing).
 *
 *  7. **Linking the Project**
 *      - Define the linking rules with appropriate flags.
 *      - For executables, link object files with the chosen linker (e.g., `g++`, `clang++`, `link.exe`).
 *      - For static libraries, ensure correct static linking rules (e.g., `ar`, `lib.exe`).
 *      - Ensure correct linking order of dependencies.
 *
 *  8. **Post-Build Actions (Optional)**
 *      - Run additional actions after the build (e.g., packaging, deployment, testing).
 *      - You can define custom Ninja rules for packaging or testing to automate post-build steps.
 *
 *  9. **Clean Up (Optional)**
 *      - Define cleanup rules to remove intermediate files (e.g., object files, temporary files).
 *      - Optionally, include a `clean` rule in the Ninja file to remove generated files.
 *
 *      Summary of When to Use Implicit Dependencies:
 *          Header files: If a source file includes a header file, mark the header as an implicit dependency.
 *          Generated files: If a source file depends on a generated file (e.g., code generation tools), mark the generated file as an implicit dependency.
 *          Configuration files: If a build depends on configuration files, mark those as implicit dependencies.
 *          External libraries: If your project uses external libraries, mark relevant headers and files as implicit dependencies.
 *          Custom scripts: Track and mark the input files used by custom build scripts as implicit dependencies.
 */
// A BuildConfiguration option that NinjaGenerator will use
enum class ProjectBuildType {
    EXECUTABLE,
    STATIC_LIBRARY,
    SHARED_LIBRARY,
    BUILD_NO_LINK // compile but do not call linker
};
enum class CompilerType {
    GCC,
    CLANG,
    MSVC
};
struct Project {
    std::string projectName;
    std::vector<std::string> sourceFiles;
    std::vector<std::string> includeDirs;
    std::vector<std::string> dependencies;
    std::string cFlags;
    std::string cxxFlags;
    std::string ldFlags;
    std::string outputPath;
    ProjectBuildType buildType;
    CompilerType compiler;
    std::string cFlagsOut;
    std::string cxxFlagsOut;
    std::string ldFlagsOut;
};

class NinjaGenerator {
public:
    NinjaGenerator() {
        const char* compiler = std::getenv("CC");
        m_writer.variable("cc", (compiler ? compiler : "g++"));
        m_writer.rule("cc", "$cc $cflags -c -o $out $in");
        m_writer.rule("ld", "$cc -o $out $in $ldflags");
        m_writer.newline();
    }
    ~NinjaGenerator() = default;

    void add(const Project& project) {
        if (m_projectNames.contains(project.projectName)) {
            throw std::runtime_error("Project name already exists.");
        }

        m_projectNames.insert(project.projectName);
        m_writer.comment(project.projectName);
        m_writer.variable("cflags", project.flags.first);
        m_writer.build(project.projectName, "ld", {std::string(project.projectName + ".o")});
        m_writer.build(std::string(project.projectName + ".o"), "cc", std::string("src/" + project.projectName + ".cpp"));
    }

    // Generate build.ninja
    void generate() const {
        std::ofstream ofs("build.ninja");
        if (!ofs.is_open()) {
            throw std::runtime_error("Failed to open build.ninja");
        }
        ofs << m_writer.string();
        ofs.close();
    }

    // Get the current value
    inline std::string string() const {
        return m_writer.string();
    }

    inline void reset() {
        m_writer.reset();
    }

private:
    NinjaWriter m_writer;
    std::unordered_set<std::string> m_projectNames;
};

#endif //NINJA_GENERATOR_HPP
