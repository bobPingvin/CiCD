#include "plugin_interface.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class LintPlugin : public PipelinePlugin {
public:
    std::string get_name() const override { return "CodeStyleLinter"; }
    
    bool handles_stage(const std::string& stage_name) const override {
        return stage_name == "lint";
    }

    bool execute_stage(const std::string& stage_name, const JsonValue& conf, std::string& error_log) override {
        std::string target_dir = conf.get_string_or("target", "./src");
        size_t max_line_len = 100;
        if (conf.contains("max_line_len") && conf.get("max_line_len").is_number()) {
            max_line_len = (size_t)conf.get("max_line_len").as_number();
        }

        std::cout << "[LINTER] Scanning directory: " << target_dir << " (max_len=" << max_line_len << ")...\n";

        if (!fs::exists(target_dir)) {
            error_log = "Target dir for lint does not exist: " + target_dir;
            return false;
        }

        bool has_errors = false;
        for (const auto& entry : fs::recursive_directory_iterator(target_dir)) {
            if (entry.path().extension() == ".cpp" || entry.path().extension() == ".hpp" || entry.path().extension() == ".h") {
                std::ifstream in(entry.path());
                std::string line;
                int line_num = 0;
                while (std::getline(in, line)) {
                    line_num++;
                    // Проверка на табуляцию
                    if (line.find('\t') != std::string::npos) {
                        std::cerr << "  [LINT ERROR] " << entry.path().filename().string()
                                  << ":" << line_num << " contains TAB character (use spaces)!\n";
                        has_errors = true;
                    }
                    // Проверка на длину строки
                    if (line.size() > max_line_len) {
                        std::cerr << "  [LINT ERROR] " << entry.path().filename().string()
                                  << ":" << line_num << " line exceeds " << max_line_len << " characters (" << line.size() << ")\n";
                        has_errors = true;
                    }
                }
            }
        }

        if (has_errors) {
            error_log = "Lint check failed due to code style violations.";
            return false;
        }

        std::cout << "[LINTER] All code style checks passed successfully!\n";
        return true;
    }
};

extern "C" PipelinePlugin* create_plugin() {
    return new LintPlugin();
}
