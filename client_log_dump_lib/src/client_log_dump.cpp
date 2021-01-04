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

#include "client_log_dump.h"

int SendLogProc::Init() {
    if ((send_info.sock_info.sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket error!");
        send_info.print_status = true;
        return -1;
    }

    bzero(&send_info.sock_info.serv_addr, sizeof(send_info.sock_info.serv_addr));
    send_info.sock_info.serv_addr.sin_family = AF_INET;
    send_info.sock_info.serv_addr.sin_port = htons(MY_PORT);
    send_info.sock_info.serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(send_info.sock_info.sockfd, (struct sockaddr *)&send_info.sock_info.serv_addr, sizeof(send_info.sock_info.serv_addr)) < 0) {
        perror("connect err");
        send_info.print_status = true;
        return -1;
    }

    if (-1 == setnonblocking(send_info.sock_info.sockfd)) {
        send_info.print_status = true;
        return -1;
    }

    return 0;
}

int SendLogProc::DeInit() {
    close(send_info.sock_info.sockfd);
    return 0;
}

int SendLogProc::setnonblocking(int sock) {
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        perror("fcntl(sock,GETFL)");
        return -1;
    }

    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
        perror("fcntl(sock,SETFL,opts)");
        return -1;
    }

    return 0;
}

int SendLogProc::SendLog(int log_level, const char * fmt, ...) {
    std::string log_level_str;
    switch (log_level) {
        case EMEKG:
            log_level_str = "EMEKG";
            break;
        case ALEKKT:
            log_level_str = "ALEKKT";
            break;
        case CRIT:
            log_level_str = "CRIT";
            break;
        case ERR:
            log_level_str = "ERR";
            break;
        case WARNING:
            log_level_str = "WARNING";
            break;
        case NOTICE:
            log_level_str = "NOTICE";
            break;
        case INFO:
            log_level_str = "INFO";
            break;
        case DEBUG:
            log_level_str = "DEBUG";
            break;
        default:
            log_level_str = "INFO";
            break;
    }

    struct tm * timenow = NULL;
    char timechar[256] = {0};
    time_t second = time(0);
    timenow = localtime(&second);
    memset(timechar, 0, sizeof(timechar));
    sprintf(timechar, " #%d-%d-%d_%d:%d:%d# ", timenow->tm_year+1900, timenow->tm_mon+1, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

    va_list arg1, arg2;
    va_start(arg1, fmt);
    va_copy(arg2, arg1);
    char buf[1+vsnprintf(NULL, 0, fmt, arg1)];
    va_end(arg1);
    vsnprintf(buf, sizeof(buf), fmt, arg2);
    va_end(arg2);

    std::string str_log = log_level_str + std::string(timechar) + buf;
    if (send_info.print_status) {
        std::cout << "not connect server LOG_PRINT: " << str_log << std::endl;
    } else {
        SendLog_tmp(str_log.c_str(), strlen(str_log.c_str()));
    }

    return 0;
}

int SendLogProc::SendLog_tmp(const char * log_str, int log_str_len) {
    while (1) {
        usleep(20000);
        int SendSize = send(send_info.sock_info.sockfd, log_str, log_str_len, 0);
        if (SendSize < 0) {
            /*
                EINTR    Indicates that when send, receive an interrupt signal, you can continue to write, or wait for the notification of subsequent epoll and select, and then write.
                EAGAIN   Indicates that when send, the write buffer queue is full, we can continue to wait, continue to write, or wait for subsequent notifications from epoll and select, and then write.
            */
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("%s:%s:%d send log error 缓冲队列已满! errno:%d \n", __FILE__ , __func__ , __LINE__ , errno);
                usleep(1000);
                continue;
            }

            printf("%s:%s:%d send log error!\n", __FILE__ , __func__ , __LINE__);
            return -1;
        }

        if (SendSize == log_str_len) {
            break;
        }

        log_str_len -= SendSize;
        log_str += SendSize;
    }

    return 0;
}
//  end file
