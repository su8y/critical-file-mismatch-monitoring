add_executable(mis_mon main.cpp
        alarm_insert_client.cpp
)

target_link_libraries(mis_mon PRIVATE
         spdlog::spdlog
         nlohmann_json::nlohmann_json
)