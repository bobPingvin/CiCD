cmake_minimum_required(VERSION 3.12)
project(CICDPipelineSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# 1. Основной исполняемый файл ci-cd
add_executable(ci-cd
    src/main.cpp
    src/pipeline.cpp
    src/process_utils.cpp
)
target_link_libraries(ci-cd PRIVATE ${CMAKE_DL_LIBS} pthread)

# 2. Плагин линтинга
add_library(lint SHARED plugins/lint/lint_plugin.cpp)
set_target_properties(lint PROPERTIES
    PREFIX "lib"
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins"
)

# 3. Плагин нотификации
add_library(notify SHARED plugins/notify/notify_plugin.cpp)
set_target_properties(notify PROPERTIES
    PREFIX "lib"
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins"
)
