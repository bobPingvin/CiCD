#pragma once
#include <string>
#include "json.hpp"

class PipelinePlugin {
public:
    virtual ~PipelinePlugin() = default;
    virtual std::string get_name() const = 0;
    
    // Вызывается перед и после каждого этапа (для нотификаций/логирования)
    virtual void on_stage_start(const std::string& stage_name) {}
    virtual void on_stage_end(const std::string& stage_name, bool success) {}
    
    // Если плагин предоставляет собственный кастомный этап пайплайна
    virtual bool handles_stage(const std::string& stage_name) const { return false; }
    virtual bool execute_stage(const std::string& stage_name, const JsonValue& stage_conf, std::string& error_log) {
        return true;
    }
};

extern "C" PipelinePlugin* create_plugin();
typedef PipelinePlugin* (*CreatePluginFunc)();
