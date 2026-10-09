#include "plugin_interface.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

class NotifyPlugin : public PipelinePlugin {
public:
    std::string get_name() const override { return "StageNotifier"; }

    void on_stage_start(const std::string& stage_name) override {
        print_stamp("STARTING", stage_name, "\033[1;36m");
    }

    void on_stage_end(const std::string& stage_name, bool success) override {
        if (success) {
            print_stamp("COMPLETED", stage_name, "\033[1;32m");
        } else {
            print_stamp("FAILED", stage_name, "\033[1;31m");
        }
    }

private:
    void print_stamp(const std::string& status, const std::string& stage, const char* color) {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm tm_buf;
        localtime_r(&now, &tm_buf);
        std::cout << color << "[NOTIFY @ " 
                  << std::put_time(&tm_buf, "%H:%M:%S") << "] Stage '" 
                  << stage << "' -> " << status << "\033[0m\n";
    }
};

extern "C" PipelinePlugin* create_plugin() {
    return new NotifyPlugin();
}
