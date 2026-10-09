#include <iostream>
#include <string>
#include "pipeline.hpp"

void print_usage() {
    std::cout << "Usage:\n"
              << "  ci-cd build --source <src_dir> --output <bin_dir>\n"
              << "  ci-cd test --tests <tests_dir>\n"
              << "  ci-cd deploy --source <bin_dir> --target <deploy_dir>\n"
              << "  ci-cd run --config <pipeline.json>\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    PipelineEngine engine;
    engine.load_plugins("./plugins");

    std::string command = argv[1];

    if (command == "build") {
        std::string source = "./src";
        std::string output = "./bin";
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--source" && i + 1 < argc) source = argv[++i];
            else if (arg == "--output" && i + 1 < argc) output = argv[++i];
        }
        return engine.execute_build(source, output) ? 0 : 1;
    }
    else if (command == "test") {
        std::string tests = "./tests";
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--tests" && i + 1 < argc) tests = argv[++i];
        }
        return engine.execute_test(tests) ? 0 : 1;
    }
    else if (command == "deploy") {
        std::string source = "./bin";
        std::string target = "./deploy";
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--source" && i + 1 < argc) source = argv[++i];
            else if (arg == "--target" && i + 1 < argc) target = argv[++i];
        }
        return engine.execute_deploy(source, target) ? 0 : 1;
    }
    else if (command == "run") {
        std::string config = "pipeline.json";
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--config" && i + 1 < argc) config = argv[++i];
        }
        return engine.run_pipeline_file(config) ? 0 : 1;
    }
    else {
        std::cerr << "Unknown command: " << command << "\n";
        print_usage();
        return 1;
    }
}
