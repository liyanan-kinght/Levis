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
#ifndef  SERVER_LOG_DUMP_SRC_SERVER_LOG_DUMP_H_
#define  SERVER_LOG_DUMP_SRC_SERVER_LOG_DUMP_H_

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <iterator>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/un.h>

#ifdef __cplusplus
}
#endif

#define MY_PORT  12345

#define MAX_EVENTS 500  //  Most processed connect

#define REVLEN  1024 * 20

#define LOG_FILE  "log_client.txt"

typedef struct socket_epool_info {
    //  Current number of connections
    int currentClient = 0;
    //  Receive buf
    char recvBuf[REVLEN] = {0};
    //  epoll descriptor
    int epollfd;
    //  Event array
    struct epoll_event eventList[MAX_EVENTS];
    struct epoll_event event;
    int sockListen;
    struct sockaddr_in server_addr;
}SOCKET_EPOOL_INFO;

typedef struct proc_recv_info {
    FILE * log_fp = NULL;
    pthread_t thread_id;
    SOCKET_EPOOL_INFO sock_epool_stru;
}PROC_RECV_INFO;

class RecvLogProc {
 public:
    int Init();
    int DeInit();

 private:
    int logfile_init();
    int logfile_deinit();
    int thread_init();
    int thread_deinit();

    void setnonblocking(int sock);
    int my_write(FILE * fp, char *read_buf, int buf_len);

    static void * thread_recv_proc(void *arg);
    void AcceptConn(int srvfd);
    int RecvData(int fd);

    PROC_RECV_INFO proc_stru;
};

#endif  //  SERVER_LOG_DUMP_SRC_SERVER_LOG_DUMP_H_
