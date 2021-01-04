/**
  ***********************************************************************************************************
  * @attention 
  * Copyright <2020> <Lenovo AI LAB>
  *
  * Redistribution and use in source and binary forms, with or without modification, are permitted provided
  * that the following conditions are met:
  * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
  * following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
  * the following disclaimer in the documentation and/or other materials provided with the distribution.
  * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
  * promote products derived from this software without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
  * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
  * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
  * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;LOSS OF USE, DATA, OR PROFITS;OR BUSINESS INTERRUPTION)
  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  * OPOSSIBILITY OF SUCH DAMAGE.
  ***********************************************************************************************************
  */
#ifndef CLIENT_LOG_DUMP_LIB_SRC_CLIENT_LOG_DUMP_H_
#define CLIENT_LOG_DUMP_LIB_SRC_CLIENT_LOG_DUMP_H_

#include <iostream>
#include <string>
#include <map>
#include <iterator>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/un.h>
#include <stdarg.h>

#ifdef __cplusplus
}
#endif


#define __SEND_LOG_TO_SERVER__  1

#if __SEND_LOG_TO_SERVER__

#define  SENDLOG_CLI_OBJ_PTR(sendlogobj, log_level_value, format, ...)  \
{ \
    sendlogobj->SendLog(log_level_value, " FILE: %s,  LINE: %d ,  func: %s " format "\n", __FILE__,  __LINE__,  __func__,  ##__VA_ARGS__); \
} \

#define  SENDLOG_CLI_OBJ(sendlogobj, log_level_value, format, ...)  \
{ \
    sendlogobj.SendLog(log_level_value, " FILE: %s,  LINE: %d ,  func: %s " format "\n", __FILE__,  __LINE__,  __func__,  ##__VA_ARGS__); \
} \

#else
#define  SENDLOG_CLI_OBJ_PTR(sendlogobj, log_level_value, format, ...)  printf(" FILE: %s,  LINE: %d ,  func: %s " format "\n", __FILE__,  __LINE__,  __func__,  ##__VA_ARGS__);
#define  SENDLOG_CLI_OBJ(sendlogobj, log_level_value, format, ...)      printf(" FILE: %s,  LINE: %d ,  func: %s " format "\n", __FILE__,  __LINE__,  __func__,  ##__VA_ARGS__);
#endif

#define MY_PORT 12345

#define UNIX_DOMAIN_SERV_PATH "/tmp/log_socket_serv"

enum VOICE_FRAME_WORK_LOGLEVEL_TYPE {
    EMEKG,   //  (Emergency): Will cause the host system to be unavailable
    ALEKKT,  //  (Warning): Problems that must be resolved immediately
    CRIT,    //  (Serious): More serious situation
    ERR,     //  (Error): An error occurred during operation
    WARNING,  //  (Reminder): Events that may affect system functions
    NOTICE,  //  (Note): Will not affect the system but it is worth noting
    INFO,    //  (Information): General information
    DEBUG,   //  (Debug): program or system debugging information, etc.
};

typedef struct socket_info_client {
    int sockfd;
    struct sockaddr_in serv_addr;
}SOCK_INFO_CLI;

typedef struct send_proc_info {
    SOCK_INFO_CLI sock_info;
    volatile bool print_status = false;
}SEND_PROC_INFO;

class SendLogProc {
 public:
    int Init();
    int DeInit();
    int SendLog(int log_level, const char * fmt, ...);

 private:
    int SendLog_tmp(const char * log_str,  int log_str_len);
    int setnonblocking(int sock);
    SEND_PROC_INFO send_info;
};

#endif  //  CLIENT_LOG_DUMP_LIB_SRC_CLIENT_LOG_DUMP_H_
