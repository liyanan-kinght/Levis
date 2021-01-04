cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_SYSTEM_NAME Linux)

option(AARCH64_ARM_COMPILER "option for aarch64 arm compiler" OFF)
option(ARMV7_COMPILER "option for arm v7 compiler" OFF)
option(OPENMP_FLAG "option for OPENMP" OFF)
option(FRAME_WORK_FLAG "option for FRAME_WORK Process" OFF)

if (AARCH64_ARM_COMPILER)
message("-- Using AARCH64_ARM_COMPILER")
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
elseif(ARMV7_COMPILER)
message("-- Using ARMV7_COMPILER")
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
else()
message("-- Using Default Gcc COMPILER")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

PROJECT(Le_micarray)

enable_language(CXX C ASM)

SET(C_FLAGS_RELEASE "-O3 -DNDEBUG")

SET(C_FLAGS_COMMON "${C_FLAGS_COMMON} -fPIC -pipe -ffast-math -funsafe-math-optimizations -ftree-vectorize")
SET(C_FLAGS_COMMON "${C_FLAGS_COMMON} -Wall -Wextra -Wpedantic -Wno-sign-compare")
SET(C_FLAGS_COMMON "${C_FLAGS_COMMON} -g -std=c11 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DHAVE_PTHREAD -Dkiss_fft_scalar=float")
SET(CXX_FLAGS_COMMON "${CXX_FLAGS_COMMON} -g -std=c++11 -ffor-scope -ffast-math -funsafe-math-optimizations -ftree-vectorize -Dkiss_fft_scalar=float")

if(ARMV7_COMPILER)
SET(C_FLAGS_COMMON "${C_FLAGS_COMMON} -march=armv7-a -mfloat-abi=hard -mfpu=neon")
SET(CXX_FLAGS_COMMON "${CXX_FLAGS_COMMON} -march=armv7-a -mfloat-abi=hard -mfpu=neon")
endif()

if (OPENMP_FLAG)
message("-- Enable OPEMMP")
SET(CXX_FLAGS_COMMON "${CXX_FLAGS_COMMON} -fopenmp")
SET(C_FLAGS_COMMON "${C_FLAGS_COMMON} -fopenmp")
endif()

SET(CMAKE_C_FLAGS_RELEASE "${C_FLAGS_RELEASE} ${C_FLAGS_COMMON}")
SET(CMAKE_CXX_FLAGS_RELEASE "${C_FLAGS_RELEASE} ${CXX_FLAGS_COMMON}")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(include)
INCLUDE_DIRECTORIES(/usr/include/python2.7)

if (FRAME_WORK_FLAG)
INCLUDE_DIRECTORIES(frame_work)
INCLUDE_DIRECTORIES(include/curl)
INCLUDE_DIRECTORIES(include/libavutil)
INCLUDE_DIRECTORIES(include/libswresample)
endif()

link_directories(libs)
link_directories(/usr/lib/python2.7)

INCLUDE_DIRECTORIES(client_log_dump_lib/src)

add_subdirectory(client_log_dump_lib)
add_subdirectory(server_log_dump)

if (FRAME_WORK_FLAG)
add_executable(Respeaker_Framework_Main  frame_work/frame_work.cpp frame_work/frame_work_main.cpp frame_work/play_audio.cpp frame_work/send_asr.cpp frame_work/cJSON.cpp frame_work/tts_proc.cpp frame_work/clock_proc.cpp frame_work/framework_volume_ctl.cpp frame_work/test_env.cpp)
target_link_libraries(Respeaker_Framework_Main  clientlogsend  le_preproc rec le_aec le_doa vad_lib le_cgmm le_trigger_awake asound curl crypto ssl mad avutil swresample pthread python2.7)
endif()

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
