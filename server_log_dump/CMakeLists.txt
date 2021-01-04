
INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(src)

add_executable(Server_Log_Dump_App  src/server_log_dump.cpp src/server_main.cpp)
target_link_libraries(Server_Log_Dump_App pthread)

