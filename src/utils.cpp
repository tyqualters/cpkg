#include "utils.hpp"

// #include <openssl/sha.h> TODO: SHA256 Function

#if _IS_WINDOWS == 1
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
// #include <unistd.h>
#include <spawn.h>
#include <wait.h>
#endif

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
    std::strcpy(argv, _argv.c_str());

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
    if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return exitCode;
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