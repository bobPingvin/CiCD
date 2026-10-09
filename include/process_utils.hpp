#pragma once
#include <string>
#include <utility>

struct ProcessResult {
    int exit_code = -1;
    std::string stdout_output;
    std::string stderr_output;
    bool success() const { return exit_code == 0; }
};

// Запуск процесса без root через POSIX fork/exec/pipe
ProcessResult execute_command(const std::string& cmd);
