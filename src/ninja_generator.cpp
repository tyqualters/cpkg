#include "ninja_generator.hpp"
#include "utils.hpp"
#include <cassert>

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
            compiler = "default";
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
    fmt::println("(Export) C Flags: {}", this->cFlagsOut);
    fmt::println("(Export) C++ Flags: {}", this->cxxFlagsOut);
    fmt::println("(Export) Linker Flags: {}", this->ldFlagsOut);
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

// Note: This function is AI generated
bool FileEqualsString(const std::string& fileName, const std::string& str) {
    if (file_exists(fileName)) {
        std::ifstream ifs(fileName);
        if (!ifs.is_open()) {
            fmt::println("Failed to open file: {}\n", fileName);
            return false;
        }
        std::string fileContent(
            (std::istreambuf_iterator<char>(ifs)),
            (std::istreambuf_iterator<char>()) // Read the entire content into a string
        );
        ifs.close();
        return fileContent == str; // Compare the whole content
    }
    return false; // If file doesn't exist, return false
}

void NinjaGenerator::generate() {
    // TODO: Fix for MSVC
    std::string cc, cxx, ld, ar;

    m_writer.comment("Global variables");

    // Check if any compilers are used
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

    // Check if no compilers are used (oh no...)
    if(!_use_msvc && !_use_gcc && !_use_clang) {
        throw std::runtime_error("No compiler selected!");
    }

    // Add if used
    if(_use_gcc) {
        fmt::println("Note: Using GCC");
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
        fmt::println("Note: Using MSVC");
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
        fmt::println("Note: Using Clang");
        cc = "clang";
        cxx = "clang++";
        ld = "clang++";
        ar = "ar";
        m_writer.variable("clang_cc", cc);
        m_writer.variable("clang_cxx", cxx);
        m_writer.variable("clang_ld", ld);
        m_writer.variable("clang_ar", ar);
    }

    m_writer.variable("cflags", "");
    m_writer.variable("cxxflags", "");
    m_writer.variable("ldflags", "");
    m_writer.newline();

    m_writer.comment("Global rules");
    m_writer.rule("clean", "rm $in", "Cleaning $in");
    if (_use_msvc) {
        // MSVC
        m_writer.rule("msvc_cc", "$msvc_cc $cflags /c $in /Fo $out", "Compiling C $in to $out");
        m_writer.rule("msvc_cxx", "$msvc_cxx $cxxflags /c $in /Fo $out", "Compiling C $in to $out");
        m_writer.rule("msvc_ld", "$msvc_ld $in $ldflags -o $out", "Linking $in to $out");
        m_writer.rule("msvc_ar", "$msvc_ar /out:$out $in", "Archiving $in to $out");
    }

    if (_use_gcc) {
        // GCC
        m_writer.rule("gcc_cc", "$gcc_cc -c $in $cflags -o $out", "Compiling $in to $out");
        m_writer.rule("gcc_cxx", "$gcc_cxx -c $in $cxxflags -o $out", "Compiling $in to $out");
        m_writer.rule("gcc_ld", "$gcc_ld $in $ldflags -o $out", "Linking $in to $out"); // TODO
        m_writer.rule("gcc_ar", "$gcc_ar rcs $out $in ", "Archiving $in to $out");
    }

    if(_use_clang) {
        // Clang
        m_writer.rule("clang_cc", "$clang_cc -c $in $cflags -o $out", "Compiling $in to $out");
        m_writer.rule("clang_cxx", "$clang_cxx -c $in $cxxflags -o $out", "Compiling $in to $out");
        m_writer.rule("clang_ld", "$clang_ld $in $ldflags -o $out", "Linking $in to $out"); // TODO
        m_writer.rule("clang_ar", "$clang_ar rcs $out $in ", "Archiving $in to $out");
    }

    auto _rule = [](const CompilerType compType, const std::string& ruleName) {
        switch(compType) {
            case CompilerType::MSVC:
                return "msvc_" + ruleName;
            case CompilerType::GCC:
                return "gcc_" + ruleName;
            case CompilerType::CLANG:
                return "clang_" + ruleName;
            default:
                throw std::runtime_error("Unknown compiler");
        }
    };

    m_writer.newline();

    // std::vector<std::string> _target_names{};
    // std::vector<std::string> _object_files{};

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
                            if (depProject.buildType == ProjectBuildType::STATIC_LIBRARY || depProject.buildType == ProjectBuildType::SHARED_LIBRARY) {
                                ldflags += " /LIBPATH:" + depProject.outputPath;
                            }
                        } else {
                            // GCC
                            cflags += " -I" + includeDir;
                            cxxflags += " -I" + includeDir;
                            if (depProject.buildType == ProjectBuildType::STATIC_LIBRARY || depProject.buildType == ProjectBuildType::SHARED_LIBRARY) {
                                ldflags += " -l" + depProject.projectName;
                            }
                        }
                    }
                    goto foundDep; // hehe goto ftw
                }
            }
            for (const auto& dependency : m_dependencies) {
                if (dependency.dependencyName == dep) {
                    cflags += " " + dependency.cFlags;
                    cxxflags += " " + dependency.cxxFlags;
                    ldflags += " " + dependency.ldFlags;
                    for (const auto& includeDir: dependency.includeDirs) {
                        if (project.compiler == CompilerType::MSVC) {
                            // MSVC
                            cflags += " /I" + includeDir;
                            cxxflags += " /I" + includeDir;
                            for (const auto& libPath : dependency.libraryPaths) {
                                ldflags += " /LIBPATH:\"" + libPath + "\"";
                            }
                        } else {
                            // GCC
                            cflags += " -I" + includeDir;
                            cxxflags += " -I" + includeDir;
                            for (const auto& libPath : dependency.libraryPaths) {
                                ldflags += " " + libPath;
                            }
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

        // Iterate thru lib dirs (not supported on MSVC)
        if(project.compiler != CompilerType::MSVC)
            for (const auto& libDir : project.libDirs) {
                 // GCC
                 ldflags += " -L" + libDir;
            }

        // Compile Object Files
        std::vector<std::string> objectFiles{};
        for (const auto& sourceFile : project.sourceFiles) {
            std::string objectFile = sourceFile.substr(0, sourceFile.find_last_of('.')) + (g_isWindows ? ".obj" : ".o");
            objectFiles.push_back(objectFile);
            if (sourceFile.ends_with(".c")) {
                m_writer.build(project.buildType == ProjectBuildType::BUILD_NO_LINK ? join_paths(project.outputPath, objectFile) : objectFile, _rule(project.compiler, "cc"), sourceFile);
                m_writer.variable("cflags", cflags, 1);
            } else {
                m_writer.build(objectFile, _rule(project.compiler, "cxx"), sourceFile);
                m_writer.variable("cxxflags", cxxflags, 1);
            }
            m_writer.newline();
        }

        // if (project.buildType != ProjectBuildType::BUILD_NO_LINK)
        //     _object_files.insert(_object_files.end(), objectFiles.begin(), objectFiles.end());

        // Link Object Files
        if (project.buildType == ProjectBuildType::EXECUTABLE) {
            m_writer.build(project.outputPath, _rule(project.compiler, "ld"), objectFiles);
            m_writer.variable("ldflags", ldflags, 1);
        } else if (project.buildType == ProjectBuildType::STATIC_LIBRARY) {
            m_writer.build(project.outputPath, _rule(project.compiler, "ar"), objectFiles);
            m_writer.variable("ldflags", ldflags, 1);
        } else if (project.buildType == ProjectBuildType::SHARED_LIBRARY) {
            std::runtime_error("Shared libraries are not yet supported.");
        } else {
            assert(project.buildType == ProjectBuildType::BUILD_NO_LINK);
        }

        m_writer.newline();
    }

    // m_writer.build("clean", "clean", _object_files);
    // Auto-Clean (do not add)
    // m_writer.build("all", "phony", "clean");
    m_writer.newline();

    if (!FileEqualsString("build.ninja", m_writer.string())) {
        std::ofstream ofs("build.ninja", std::ios::trunc);
        if (!ofs.is_open()) {
            throw std::runtime_error("Failed to open build.ninja");
        }
        ofs << m_writer.string();
        ofs.close();
    } else {
        fmt::println("build.ninja is up to date.");
    }

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