#pragma once
#include <string>
#include <vector>
#include <memory>
#include "json.hpp"
#include "plugin_interface.hpp"

struct PluginHandle {
    void* handle = nullptr;
    std::unique_ptr<PipelinePlugin> plugin;
};

class PipelineEngine {
public:
    PipelineEngine();
    ~PipelineEngine();

    void load_plugins(const std::string& plugins_dir);
    bool execute_build(const std::string& source_dir, const std::string& output_dir);
    bool execute_test(const std::string& tests_dir);
    bool execute_deploy(const std::string& source_dir, const std::string& target_dir);
    bool execute_script(const std::string& script_path);
    bool run_pipeline_file(const std::string& json_path);

private:
    std::vector<PluginHandle> plugins;

    void notify_start(const std::string& stage);
    void notify_end(const std::string& stage, bool success);
    bool run_custom_plugin_stage(const std::string& stage_name, const JsonValue& conf);
};
