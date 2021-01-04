
INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(src)

add_library(clientlogsend SHARED src/client_log_dump.cpp src/client_log_dump.h)


