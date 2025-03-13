#ifndef NINJA_GENERATOR_HPP
#define NINJA_GENERATOR_HPP

// C++ STL
#include <fstream>
#include <unordered_set>

// First Party
#include "ninja_syntax.hpp"

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
        m_writer.variable("cflags", project.compilerFlags);
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
