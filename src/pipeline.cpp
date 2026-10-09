#include "pipeline.hpp"
#include "process_utils.hpp"
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <sys/stat.h>

namespace fs = std::filesystem;

PipelineEngine::PipelineEngine() = default;

PipelineEngine::~PipelineEngine() {
    for (auto& p : plugins) {
        p.plugin.reset();
        if (p.handle) {
            dlclose(p.handle);
        }
    }
    plugins.clear();
}

void PipelineEngine::load_plugins(const std::string& plugins_dir) {
    if (!fs::exists(plugins_dir)) return;

    for (const auto& entry : fs::directory_iterator(plugins_dir)) {
        if (entry.path().extension() == ".so") {
            void* handle = dlopen(entry.path().c_str(), RTLD_NOW);
            if (!handle) {
                std::cerr << "[WARN] Failed to load plugin " << entry.path() << ": " << dlerror() << "\n";
                continue;
            }

            dlerror(); // сброс ошибок
            auto create_func = (CreatePluginFunc)dlsym(handle, "create_plugin");
            const char* dlsym_error = dlerror();
            if (dlsym_error) {
                std::cerr << "[WARN] Cannot find create_plugin in " << entry.path() << ": " << dlsym_error << "\n";
                dlclose(handle);
                continue;
            }

            PipelinePlugin* raw_ptr = create_func();
            if (raw_ptr) {
                plugins.push_back({handle, std::unique_ptr<PipelinePlugin>(raw_ptr)});
                std::cout << "[SYSTEM] Loaded plugin: " << raw_ptr->get_name() 
                          << " from " << entry.path().filename() << "\n";
            }
        }
    }
}

void PipelineEngine::notify_start(const std::string& stage) {
    for (auto& p : plugins) {
        p.plugin->on_stage_start(stage);
    }
}

void PipelineEngine::notify_end(const std::string& stage, bool success) {
    for (auto& p : plugins) {
        p.plugin->on_stage_end(stage, success);
    }
}

bool PipelineEngine::run_custom_plugin_stage(const std::string& stage_name, const JsonValue& conf) {
    for (auto& p : plugins) {
        if (p.plugin->handles_stage(stage_name)) {
            std::string err;
            notify_start(stage_name);
            bool ok = p.plugin->execute_stage(stage_name, conf, err);
            notify_end(stage_name, ok);
            if (!ok) {
                std::cerr << "[ERROR] Plugin stage '" << stage_name << "' failed: " << err << "\n";
            }
            return ok;
        }
    }
    return false;
}

bool PipelineEngine::execute_build(const std::string& source_dir, const std::string& output_dir) {
    notify_start("build");
    if (!fs::exists(source_dir)) {
        std::cerr << "[BUILD ERROR] Source directory '" << source_dir << "' does not exist.\n";
        notify_end("build", false);
        return false;
    }
    fs::create_directories(output_dir);

    std::string sources;
    for (const auto& entry : fs::directory_iterator(source_dir)) {
        if (entry.path().extension() == ".cpp") {
            sources += entry.path().string() + " ";
        }
    }

    if (sources.empty()) {
        std::cerr << "[BUILD ERROR] No .cpp source files found in " << source_dir << "\n";
        notify_end("build", false);
        return false;
    }

    std::string bin_target = output_dir + "/app";
    std::string compile_cmd = "g++ -std=c++17 -O2 " + sources + " -o " + bin_target;

    std::cout << "[BUILD] Executing: " << compile_cmd << "\n";
    ProcessResult res = execute_command(compile_cmd);

    if (!res.success()) {
        std::cerr << "[BUILD ERROR] Compilation failed with exit code " << res.exit_code << "\n";
        std::cerr << "[BUILD STDERR]\n" << res.stderr_output << "\n";
        notify_end("build", false);
        return false;
    }

    std::cout << "[BUILD SUCCESS] Binary built at: " << bin_target << "\n";
    notify_end("build", true);
    return true;
}

bool PipelineEngine::execute_test(const std::string& tests_dir) {
    notify_start("test");
    if (!fs::exists(tests_dir)) {
        std::cerr << "[TEST ERROR] Tests directory '" << tests_dir << "' does not exist.\n";
        notify_end("test", false);
        return false;
    }

    std::string test_sources;
    for (const auto& entry : fs::directory_iterator(tests_dir)) {
        if (entry.path().extension() == ".cpp") {
            test_sources += entry.path().string() + " ";
        }
    }

    if (test_sources.empty()) {
        std::cerr << "[TEST ERROR] No test files (.cpp) found in " << tests_dir << "\n";
        notify_end("test", false);
        return false;
    }

    fs::create_directories("./bin");
    std::string runner_bin = "./bin/test_runner";
    std::string compile_cmd = "g++ -std=c++17 -Iinclude -pthread " + test_sources + " -o " + runner_bin;

    std::cout << "[TEST] Building unit test runner: " << compile_cmd << "\n";
    ProcessResult build_res = execute_command(compile_cmd);
    if (!build_res.success()) {
        std::cerr << "[TEST BUILD ERROR] Compilation of tests failed:\n" << build_res.stderr_output << "\n";
        notify_end("test", false);
        return false;
    }

    std::cout << "[TEST] Executing test runner...\n";
    ProcessResult run_res = execute_command(runner_bin);
    std::cout << run_res.stdout_output;
    if (!run_res.stderr_output.empty()) {
        std::cerr << run_res.stderr_output;
    }

    if (!run_res.success()) {
        std::cerr << "[TEST ERROR] Tests failed with exit code " << run_res.exit_code << "\n";
        notify_end("test", false);
        return false;
    }

    notify_end("test", true);
    return true;
}

bool PipelineEngine::execute_deploy(const std::string& source_dir, const std::string& target_dir) {
    notify_start("deploy");
    if (!fs::exists(source_dir)) {
        std::cerr << "[DEPLOY ERROR] Source bin dir does not exist: " << source_dir << "\n";
        notify_end("deploy", false);
        return false;
    }

    try {
        fs::create_directories(target_dir);
        std::vector<std::string> copied_files;

        for (const auto& entry : fs::directory_iterator(source_dir)) {
            if (fs::is_regular_file(entry.status())) {
                auto dest = fs::path(target_dir) / entry.path().filename();
                fs::copy_file(entry.path(), dest, fs::copy_options::overwrite_existing);
                // выставление прав на исполнение
                fs::permissions(dest, fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
                                      fs::perms::others_read | fs::perms::others_exec,
                                fs::perm_options::replace);
                copied_files.push_back(entry.path().filename().string());
            }
        }

        // Создание отчёта о деплое
        std::ofstream report(target_dir + "/deploy_report.txt");
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        report << "================ CI/CD DEPLOYMENT REPORT ================\n";
        report << "Timestamp: " << std::ctime(&now);
        report << "Status: SUCCESSFUL\n";
        report << "Target Directory: " << target_dir << "\n";
        report << "Deployed Files (" << copied_files.size() << "):\n";
        for (const auto& f : copied_files) {
            report << "  - " << f << "\n";
        }
        report << "=========================================================\n";
        report.close();

        std::cout << "[DEPLOY] Deployment finished. Report written to " << target_dir << "/deploy_report.txt\n";
    } catch (const std::exception& e) {
        std::cerr << "[DEPLOY ERROR] Deployment failed: " << e.what() << "\n";
        notify_end("deploy", false);
        return false;
    }

    notify_end("deploy", true);
    return true;
}

bool PipelineEngine::execute_script(const std::string& script_path) {
    notify_start("script");
    if (!fs::exists(script_path)) {
        std::cerr << "[SCRIPT ERROR] Script does not exist: " << script_path << "\n";
        notify_end("script", false);
        return false;
    }

    // Выставление chmod +x без sudo
    chmod(script_path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    std::cout << "[SCRIPT] Executing user script: " << script_path << "\n";
    ProcessResult res = execute_command(script_path);

    if (!res.stdout_output.empty()) std::cout << res.stdout_output;
    if (!res.stderr_output.empty()) std::cerr << res.stderr_output;

    if (!res.success()) {
        std::cerr << "[SCRIPT ERROR] Script exited with code " << res.exit_code << "\n";
        notify_end("script", false);
        return false;
    }

    notify_end("script", true);
    return true;
}

bool PipelineEngine::run_pipeline_file(const std::string& json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        std::cerr << "[PIPELINE ERROR] Could not open config file: " << json_path << "\n";
        return false;
    }
    std::stringstream ss;
    ss << file.rdbuf();

    JsonValue root;
    try {
        JsonParser parser(ss.str());
        root = parser.parse_value();
    } catch (const std::exception& e) {
        std::cerr << "[PIPELINE ERROR] JSON parse error: " << e.what() << "\n";
        return false;
    }

    if (!root.contains("pipeline") || !root.get("pipeline").is_array()) {
        std::cerr << "[PIPELINE ERROR] Config must contain 'pipeline' array.\n";
        return false;
    }

    const auto& stages = root.get("pipeline").as_array();
    std::cout << "[PIPELINE] Starting execution (" << stages.size() << " stage(s))...\n";

    for (size_t i = 0; i < stages.size(); ++i) {
        const auto& item = stages[i];
        if (!item.contains("stage")) {
            std::cerr << "[PIPELINE ERROR] Stage #" << i << " missing 'stage' field.\n";
            return false;
        }
        std::string stage_name = item.get("stage").as_string();
        std::cout << "\n>>> [STAGE " << (i + 1) << "/" << stages.size() << "] " << stage_name << " <<<\n";

        bool ok = true;
        if (stage_name == "build") {
            std::string src = item.get_string_or("source", "./src");
            std::string out = item.get_string_or("output", "./bin");
            ok = execute_build(src, out);
        } else if (stage_name == "test") {
            std::string tests = item.get_string_or("tests", "./tests");
            ok = execute_test(tests);
        } else if (stage_name == "deploy") {
            std::string src = item.get_string_or("source", "./bin");
            std::string trg = item.get_string_or("target", "./deploy");
            ok = execute_deploy(src, trg);
        } else if (stage_name == "script") {
            std::string script = item.get_string_or("script", "");
            ok = execute_script(script);
        } else {
            // Проверка плагинов на кастомный этап
            bool handled = false;
            for (auto& p : plugins) {
                if (p.plugin->handles_stage(stage_name)) {
                    handled = true;
                    ok = run_custom_plugin_stage(stage_name, item);
                    break;
                }
            }
            if (!handled) {
                std::cerr << "[PIPELINE ERROR] Unknown stage: " << stage_name << "\n";
                ok = false;
            }
        }

        if (!ok) {
            std::cerr << "\n[PIPELINE ABORTED] Pipeline failed at stage '" << stage_name << "'. Halting.\n";
            return false;
        }
    }

    std::cout << "\n============================================\n";
    std::cout << "[PIPELINE FINISHED] All stages completed successfully!\n";
    std::cout << "============================================\n";
    return true;
}
