#include "ninja_generator.hpp"
#include "utils.hpp"

void Project::print() const {

    std::string buildType;
    switch (this->buildType) {
        case ProjectBuildType::EXECUTABLE:
            buildType = "executable";
        break;
        case ProjectBuildType::SHARED_LIBRARY:
            buildType = "shared";
        break;
        case ProjectBuildType::STATIC_LIBRARY:
            buildType = "static";
        break;
        case ProjectBuildType::BUILD_NO_LINK:
            [[fallthrough]];
        default:
            buildType = "object";
    }

    std::string compiler;
    switch (this->compiler) {
        case CompilerType::GCC:
            compiler = "gcc";
        break;
        case CompilerType::CLANG:
            compiler = "clang";
        break;
        case CompilerType::MSVC:
            compiler = "msvc";
        break;
        default:
            compiler = "unknown";
    }

    fmt::println("Project: {} version {}", this->projectName, this->version);
    fmt::println("Build Type: {}", buildType);
    fmt::println("Compiler: {}", compiler);
    fmt::println("Source Files: {}", fmt::join(this->sourceFiles, ", "));
    fmt::println("Include Directories: {}", fmt::join(this->includeDirs, ", "));
    fmt::println("Library Directories: {}", fmt::join(this->libDirs, ", "));
    fmt::println("C Flags: {}", this->cFlags);
    fmt::println("C++ Flags: {}", this->cxxFlags);
    fmt::println("Linker Flags: {}", this->ldFlags);
    fmt::println("Output Path: {}", this->outputPath);
    fmt::println("(Import) C Flags: {}", this->cFlagsOut);
    fmt::println("(Import) C++ Flags: {}", this->cxxFlagsOut);
    fmt::println("(Import) Linker Flags: {}", this->ldFlagsOut);
    fmt::println("Dependencies:");
    for (const auto& dep : this->dependencies) {
        fmt::println("  {}", dep);
    }
    fmt::println("");
}

std::string Project::string() const {

    // UNLESS -rb or --rebuild is used in place of -b or --build
    return "(NOT IMPLEMENTED YET)"; // TODO: Return JSON and then compare each project for changes in .build_cpkg.json before writing build.ninja
}


std::vector<std::string> FindHeadersInFile(const std::string fileName) {
    if (file_exists(fileName)) {
        std::ifstream ifs(fileName);
        std::string line;
        std::vector<std::string> headers;
        while (std::getline(ifs, line)) {
            if (line.find("#include") != std::string::npos) {
                size_t start = line.find("\"");
                size_t end = line.find("\"", start + 1);
                if (start != std::string::npos && end != std::string::npos) {
                    headers.push_back(line.substr(start + 1, end - start - 1));
                }
            }
        }
        ifs.close();
        return headers;
    } else throw std::runtime_error("File not found.");
}

void NinjaGenerator::generate() {

    // TODO: Iterate thru each Project
    //  Add the compiler to rules (i.e., gcc_cc)
    //  Choose to build with the compiler

    // Default to MSVC on Windows
    // Default to GCC on Linux

    std::string cc, cxx, ld, ar;

    if constexpr(g_isWindows) {

    } else {

    }

    m_writer.comment("Global variables");

    bool _use_msvc = false, _use_gcc = false, _use_clang = false;
    for(const auto& project : m_projects) {
        switch(project.compiler) {
            case CompilerType::GCC:
                _use_gcc = true;
            break;
            case CompilerType::MSVC:
                _use_msvc = true;
            break;
            case CompilerType::CLANG:
                _use_clang = true;
            break;
            default:
                throw std::runtime_error("Unknown compiler");
        }
    }

    if(!_use_msvc && !_use_gcc && !_use_clang) {
        throw std::runtime_error("No compiler selected!");
    }

    // Add if used
    if(_use_gcc) {
        cc = "gcc";
        cxx = "g++";
        ld = "g++";
        ar = "ar";
        m_writer.variable("gcc_cc", cc);
        m_writer.variable("gcc_cxx", cxx);
        m_writer.variable("gcc_ld", ld);
        m_writer.variable("gcc_ar", ar);
    }

    // Add if used
    if(_use_msvc) {
        cc = "cl";
        cxx = "cl";
        ld = "link";
        ar = "lib";
        m_writer.variable("msvc_cc", cc);
        m_writer.variable("msvc_cxx", cxx);
        m_writer.variable("msvc_ld", ld);
        m_writer.variable("msvc_ar", ar);
    }

    // Add if used
    if(_use_clang) {
        cc = "clang";
        cxx = "clang++";
        ld = "clang++";
        ar = "ar";
        m_writer.variable("clang_cc", cc);
        m_writer.variable("clang_cxx", cxx);
        m_writer.variable("clang_ld", ld);
        m_writer.variable("clang_ar", ar);
    }

    // Default values (cc, cxx, ld, ar)
    if constexpr(g_isWindows) {
        // use MSVC or use next avail.
    } else {
        // use GCC or use clang
    }

    m_writer.variable("cc", cc);
    m_writer.variable("cxx", cxx);
    m_writer.variable("ld", ld);
    m_writer.variable("ar", ar);

    m_writer.variable("cflags", "");
    m_writer.variable("ldflags", "");
    m_writer.newline();

    m_writer.comment("Global rules");
    if (_use_msvc) {
        // MSVC
        m_writer.rule("msvc_cc", "$cxx /c $in $cflags /Fo $out", "Compiling $in to $out");
        m_writer.rule("msvc_ld", "$ld $in $ldflags -o $out", "Linking $in to $out");
        m_writer.rule("msvc_ar", "/out:$out $in", "Archiving $in to $out");
    }

    if (_use_gcc) {
        // GCC
        m_writer.rule("gcc_cc", "$cxx -c $in $cflags -o $out", "Compiling $in to $out");
        m_writer.rule("gcc_ld", "$ld $in $ldflags -o $out", "Linking $in to $out"); // TODO
        m_writer.rule("gcc_ar", "$ar rcs $out $in ", "Archiving $in to $out");
    }

    if(_use_clang) {
        // Clang
        m_writer.rule("clang_cc", "$cxx -c $in $cflags -o $out", "Compiling $in to $out");
        m_writer.rule("clang_ld", "$ld $in $ldflags -o $out", "Linking $in to $out"); // TODO
        m_writer.rule("clang_ar", "$ar rcs $out $in ", "Archiving $in to $out");
    }

    // Default rules (cc, cxx, ld, ar)
    if constexpr(g_isWindows) {
        // use MSVC or next avail.
    } else {
        // use GCC or clang
    }

    m_writer.newline();

    // Step 1. Source files
    for (const auto& project : m_projects) {
        m_writer.comment(project.projectName);
        std::string cflags = project.cFlags,
            cxxflags = project.cxxFlags,
            ldflags = project.ldFlags;

        // Iterate thru dependencies
        for (const auto& dep : project.dependencies) {
            for (const auto& depProject : m_projects) {
                if (depProject.projectName == dep) {
                    cflags += " " + depProject.cFlagsOut;
                    cxxflags += " " + depProject.cxxFlagsOut;
                    ldflags += " " + depProject.ldFlagsOut;
                    for (const auto& includeDir: depProject.includeDirs) {
                        if (project.compiler == CompilerType::MSVC) {
                            // MSVC
                            cflags += " /I" + includeDir;
                            cxxflags += " /I" + includeDir;
                        } else {
                            // GCC
                            cflags += " -I" + includeDir;
                            cxxflags += " -I" + includeDir;
                        }
                    }
                    goto foundDep; // hehe goto ftw
                }
            }
            fmt::println("Missing dependency: {}", dep);
            throw std::runtime_error("Dependency not found.");
foundDep:
            continue; // required here for msvc
        }

        // Add compiler
        if constexpr(g_isWindows) {
            if(project.compiler != CompilerType::MSVC) {
                switch(project.compiler) {
                    case CompilerType::GCC:
                        m_writer.variable("cc", "gcc", 1);
                        m_writer.variable("cxx", "g++", 1);
                        m_writer.variable("ld", "g++", 1);
                        m_writer.variable("ar", "ar", 1);
                        break;
                    case CompilerType::CLANG:
                        m_writer.variable("cc", "clang", 1);
                        m_writer.variable("cxx", "clang++", 1);
                        m_writer.variable("ld", "clang++", 1);
                        m_writer.variable("ar", "ar", 1);
                        break;
                    default:
                        throw std::runtime_error("Unknown compiler.");
                }
            }
        } else {
            if(project.compiler != CompilerType::GCC) {
                // NOTE: CANNOT BE MSVC
                switch(project.compiler) {
                    case CompilerType::CLANG:
                        m_writer.variable("cc", "clang", 1);
                        m_writer.variable("cxx", "clang++", 1);
                        m_writer.variable("ld", "clang++", 1);
                        m_writer.variable("ar", "ar", 1);
                        break;
                    default:
                        throw std::runtime_error("Unknown compiler.");
                }
            }
        }

        // Iterate thru include dirs
        for (const auto& includeDir : project.includeDirs) {
            if (project.compiler == CompilerType::MSVC) {
                // MSVC
                cflags += " /I" + includeDir;
                cxxflags += " /I" + includeDir;
            } else {
                // GCC
                cflags += " -I" + includeDir;
                cxxflags += " -I" + includeDir;
            }
        }

        // Iterate thru lib dirs
        for (const auto& libDir : project.libDirs) {
            ldflags += " -L" + libDir;
        }

        // Compile Object Files
        std::vector<std::string> objectFiles{};
        for (const auto& sourceFile : project.sourceFiles) {
            std::string objectFile = sourceFile.substr(0, sourceFile.find_last_of('.')) + (g_isWindows ? ".obj" : ".o");
            objectFiles.push_back(objectFile);
            m_writer.build(objectFile, "cc", sourceFile);
            m_writer.variable("cflags", cflags, 1);
            m_writer.variable("cxxflags", cxxflags, 1);
            m_writer.newline();
        }

        // Link Object Files
        if (project.buildType == ProjectBuildType::EXECUTABLE) {
            m_writer.build(NinjaGenerator::ProjectNameToFileName(project.projectName, project.buildType), "ld", objectFiles);
            m_writer.variable("ldflags", ldflags, 1);
        } else if (project.buildType == ProjectBuildType::STATIC_LIBRARY) {
            m_writer.build(NinjaGenerator::ProjectNameToFileName(project.projectName, project.buildType), "ar", objectFiles);
            m_writer.variable("ldflags", ldflags, 1);
        }

        m_writer.newline();
    }

    std::ofstream ofs("build.ninja");
    if (!ofs.is_open()) {
        throw std::runtime_error("Failed to open build.ninja");
    }
    ofs << m_writer.string();
    ofs.close();
}

std::string NinjaGenerator::ProjectNameToFileName(const std::string& projectName, const ProjectBuildType buildType) {
    switch (buildType) {
        case ProjectBuildType::EXECUTABLE:
            if constexpr (g_isWindows) {
                return projectName + ".exe";
            } else return projectName;
        case ProjectBuildType::STATIC_LIBRARY:
            if constexpr (g_isWindows) {
                return "lib" + projectName + ".lib";
            } else return "lib" + projectName + ".a";
        case ProjectBuildType::SHARED_LIBRARY:
            if constexpr (g_isWindows) {
                return "lib" + projectName + ".dll";
            } else return "lib" + projectName + ".so";
        case ProjectBuildType::BUILD_NO_LINK:
            [[fallthrough]];
        default:
            if constexpr (g_isWindows) {
                return projectName + ".obj";
            } else return projectName + ".o";
    }
}