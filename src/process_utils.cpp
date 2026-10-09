#include "process_utils.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <array>

ProcessResult execute_command(const std::string& cmd) {
    ProcessResult result;
    int out_pipe[2];
    int err_pipe[2];

    if (pipe(out_pipe) != 0 || pipe(err_pipe) != 0) {
        result.stderr_output = "Failed to create POSIX pipes";
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        result.stderr_output = "fork() failed";
        return result;
    }

    if (pid == 0) { // Дочерний процесс
        close(out_pipe[0]);
        close(err_pipe[0]);
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);
        close(out_pipe[1]);
        close(err_pipe[1]);

        execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);
        _exit(127);
    }

    // Родительский процесс
    close(out_pipe[1]);
    close(err_pipe[1]);

    std::array<char, 512> buffer;
    ssize_t bytes_read;

    while ((bytes_read = read(out_pipe[0], buffer.data(), buffer.size())) > 0) {
        result.stdout_output.append(buffer.data(), bytes_read);
    }
    close(out_pipe[0]);

    while ((bytes_read = read(err_pipe[0], buffer.data(), buffer.size())) > 0) {
        result.stderr_output.append(buffer.data(), bytes_read);
    }
    close(err_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else {
        result.exit_code = -1;
    }

    return result;
}
