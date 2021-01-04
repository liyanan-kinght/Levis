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

#include "server_log_dump.h" 

int RecvLogProc::Init() {
    if (-1 == logfile_init()) {
        return -1;
    }

    if ((proc_stru.sock_epool_stru.sockListen = socket(AF_INET,  SOCK_STREAM,  0)) < 0) {
        printf("%s:%s:%d ERROR [socket error!]\n",  __FILE__,  __func__,  __LINE__);
        return -1;
    }

    setnonblocking(proc_stru.sock_epool_stru.sockListen);

    bzero(&proc_stru.sock_epool_stru.server_addr,  sizeof(proc_stru.sock_epool_stru.server_addr));
    proc_stru.sock_epool_stru.server_addr.sin_family = AF_INET;
    proc_stru.sock_epool_stru.server_addr.sin_port = htons(MY_PORT);
    proc_stru.sock_epool_stru.server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(proc_stru.sock_epool_stru.sockListen,  (struct sockaddr*)&proc_stru.sock_epool_stru.server_addr,  sizeof(proc_stru.sock_epool_stru.server_addr)) < 0) {
        perror("bind err");
        printf("%s:%s:%d ERROR [bind error!]\n",  __FILE__,  __func__,  __LINE__);
        return -1;
    }

    if (listen(proc_stru.sock_epool_stru.sockListen,  10) < 0) {
        printf("%s:%s:%d ERROR [listen error!]\n",  __FILE__,  __func__,  __LINE__);
        return -1;
    }

    //   epoll initialization
    proc_stru.sock_epool_stru.epollfd = epoll_create(MAX_EVENTS);
    proc_stru.sock_epool_stru.event.events = EPOLLIN|EPOLLET;
    proc_stru.sock_epool_stru.event.data.fd = proc_stru.sock_epool_stru.sockListen;

    //  add Event
    if (epoll_ctl(proc_stru.sock_epool_stru.epollfd,  EPOLL_CTL_ADD,  proc_stru.sock_epool_stru.sockListen,  &proc_stru.sock_epool_stru.event) < 0) {
        printf("%s:%s:%d ERROR [epoll add fail : fd = %d}\n",  __FILE__,  __func__,  __LINE__,  proc_stru.sock_epool_stru.sockListen);
        return -1;
    }

    if (-1 == thread_init()) {
        return -1;
    }

    return 0;
}

int RecvLogProc::DeInit() {
    thread_deinit();
    logfile_init();
    close(proc_stru.sock_epool_stru.epollfd);
    close(proc_stru.sock_epool_stru.sockListen);
    return 0;
}

int RecvLogProc::logfile_init() {
    proc_stru.log_fp = fopen(LOG_FILE, "wb");
    if (NULL == proc_stru.log_fp) {
        printf("%s:%s:%d open client log file err!\n",  __FILE__,  __func__,  __LINE__);
        return -1;
    }

    return 0;
}

int RecvLogProc::logfile_deinit() {
    if (NULL != proc_stru.log_fp) {
        fclose(proc_stru.log_fp);
        proc_stru.log_fp = NULL;
    }

    return 0;
}

int RecvLogProc::thread_init() {
    int ret = 0;
    ret = pthread_create(&proc_stru.thread_id,  NULL,  thread_recv_proc,  this);  //  读取主线程数据的asr线程
    if (0 != ret) {
        printf("%s:%d:%s: ERROR[thread create error!]\n", __FILE__,  __LINE__,  __func__);
        return -1;
    }

    return 0;
}

int RecvLogProc::thread_deinit() {
    pthread_join(proc_stru.thread_id,  NULL);
    printf("%s:%d:%s: tts thread deinit success!\n", __FILE__,  __LINE__,  __func__);

    return 0;
}

void * RecvLogProc::thread_recv_proc(void *arg) {
    RecvLogProc *obj = reinterpret_cast<RecvLogProc*>(arg);

    while (1) {
        int ret = epoll_wait(obj->proc_stru.sock_epool_stru.epollfd,  obj->proc_stru.sock_epool_stru.eventList,  MAX_EVENTS,  -1);

        if (ret < 0) {
            std::cout << "epoll fault : " << strerror(errno) << std::endl;
            printf("%s:%s:%d ERROR [epoll wait error!]\n",  __FILE__,  __func__,  __LINE__);
            break;
        } else if (ret == 0) {
            printf("%s:%s:%d ERROR [epoll wait timeout!]\n",  __FILE__,  __func__,  __LINE__);
            continue;
        }

        for (int i=0; i < ret; i++) {
            //  Error exit
            if ((obj->proc_stru.sock_epool_stru.eventList[i].events & EPOLLERR) || (obj->proc_stru.sock_epool_stru.eventList[i].events & EPOLLHUP) || !(obj->proc_stru.sock_epool_stru.eventList[i].events & EPOLLIN)) {
                if (obj->proc_stru.sock_epool_stru.eventList[i].events & EPOLLERR) {
                    std::cout << "EPILLERR" << std::endl;
                }

                if (obj->proc_stru.sock_epool_stru.eventList[i].events & EPOLLHUP) {
                    std::cout << "EPOLLHUP" << std::endl;
                }

                if (!(obj->proc_stru.sock_epool_stru.eventList[i].events & EPOLLIN)) {
                    std::cout << "!epollin" << std::endl;
                }

                printf("%s:%s:%d close fd:%d\n",  __FILE__,  __func__,  __LINE__, obj->proc_stru.sock_epool_stru.eventList[i].data.fd);
                close(obj->proc_stru.sock_epool_stru.eventList[i].data.fd);
                epoll_ctl(obj->proc_stru.sock_epool_stru.epollfd, EPOLL_CTL_DEL, obj->proc_stru.sock_epool_stru.eventList[i].data.fd , NULL);
                break;
            }

            if (obj->proc_stru.sock_epool_stru.eventList[i].data.fd == obj->proc_stru.sock_epool_stru.sockListen) {
                obj->AcceptConn(obj->proc_stru.sock_epool_stru.sockListen);
            } else {
                obj->RecvData(obj->proc_stru.sock_epool_stru.eventList[i].data.fd);
            }
        }
    }
    pthread_exit(NULL);
}

void RecvLogProc::setnonblocking(int sock) {
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        perror("fcntl(sock, GETFL)");
        exit(1);
    }

    opts = opts|O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
        perror("fcntl(sock, SETFL, opts)");
        exit(1);
    }
}

int RecvLogProc::my_write(FILE * fp, char *read_buf, int buf_len) {
    int total_num = 0;
    int wr_num = 0;

    fflush(fp);
    while (buf_len != total_num) {
        wr_num = fwrite(read_buf + total_num,  sizeof(char),  buf_len - total_num,  fp);
        total_num += wr_num;
    }
    fflush(fp);

    return 0;
}

void RecvLogProc::AcceptConn(int srvfd) {
    struct sockaddr_in sin;
    socklen_t len = sizeof(struct sockaddr_in);
    bzero(&sin,  len);

    int confd = accept(srvfd,  (struct sockaddr*)&sin,  &len);

    if (confd < 0) {
        perror(" accrpt err ");
        printf("%s:%s:%d ERROR [bad accept!]\n",  __FILE__,  __func__,  __LINE__);
        return;
    } else {
        printf("%s:%s:%d Accept Connection: %d\n",  __FILE__,  __func__,  __LINE__,  confd);
    }

    setnonblocking(confd);

    //  Add the newly established connection to the monitoring of EPOLL
    struct epoll_event event;
    event.data.fd = confd;
    event.events =  EPOLLIN|EPOLLET;
    epoll_ctl(proc_stru.sock_epool_stru.epollfd,  EPOLL_CTL_ADD,  confd,  &event);
}

int RecvLogProc::RecvData(int sockfd) {
    int ret = 0;

    while (1) {
        memset(proc_stru.sock_epool_stru.recvBuf,  0,  REVLEN);
        ret = recv(sockfd,  reinterpret_cast<char *>(proc_stru.sock_epool_stru.recvBuf),  REVLEN,  0);
        printf("%s:%s:%d server recv log msg size:%d\n",  __FILE__,  __func__,  __LINE__,  ret);
        if (0 == ret) {
            perror(" ret:0 recv err ");
            close(sockfd);
            epoll_ctl(proc_stru.sock_epool_stru.epollfd, EPOLL_CTL_DEL, sockfd, NULL);

            printf("%s:%s:%d close connfd:%d ret:%d data is %s\n",  __FILE__,  __func__,  __LINE__,  sockfd,  ret,  proc_stru.sock_epool_stru.recvBuf);
            return -1;
        } else if (ret < 0) {
            if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
                /*
                    buflen > 0 && errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR,
                    Indicates that there is no data temporarily. (Read/write is the same)
                */
                printf("%s:%s:%d  connfd:%d , no data for read! \n",  __FILE__,  __func__,  __LINE__,  sockfd);
                break;
            }

            perror(" ret < 0 recv err ");
            close(sockfd);
            epoll_ctl(proc_stru.sock_epool_stru.epollfd, EPOLL_CTL_DEL, sockfd, NULL);
            printf("%s:%s:%d ECONNRESET close connfd:%d ret:%d\n",  __FILE__,  __func__,  __LINE__,  sockfd , ret);
            return -1;
        } else {
            my_write(proc_stru.log_fp, proc_stru.sock_epool_stru.recvBuf, ret);

            if (REVLEN == ret) {
                //  nothing
            } else {
            break;
            }
        }
    }

    return 0;
}

