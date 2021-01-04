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
#ifndef FRAME_WORK_SEND_ASR_H_
#define FRAME_WORK_SEND_ASR_H_

#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cstdio>

#include "frame_work_common_def.h"
#include "client_log_dump.h"

#define AWAKE_WAIT_TIME_NUM 400
#define AWAKE_WAIT_TIME_NUM_MAX 600
#define CACHE_BUF_NUM 50
#define SEND_500MS_BUF_NUM 20

#define POSTASRURL  "https://voice.lenovomm.com/lasf/asr"

typedef struct send_500ms_queue {
    struct send_500ms_queue *next;
    char *data = NULL;
    bool vadflag = true;
}SEND_500MS_QUEUE;

typedef struct memory_stru {
    char *memory = NULL;
    size_t size = 0;
}MEMORY_STRU_URL_RECV;

typedef struct curl_asr {
    CURL *_pCurl = NULL;
    struct curl_slist * _header = NULL;
    struct curl_slist * _list_par = NULL;
    struct curl_slist * _list_voi = NULL;
    struct curl_httppost * _formpost = NULL;
    struct curl_httppost * _last_ptr = NULL;
    char _curl_stime[14] = {0};
    char asr_response_buf[ASR_RESPONSE_MAX_MSG + 1] = {0};
}CURL_ASR;

typedef struct asr_mutex_and_cond {
    pthread_mutex_t asr_send_mutex;
    pthread_cond_t asr_cond_process;
}ASR_MUTEX_AND_COND;

typedef struct cache_10ms_info {
    int send_id = 0;
    int copy_id = 0;
    int cache_num = 0;
    int cache_total_count = 0;
    bool already_get_500ms_buf = false;
    SEND_500MS_QUEUE *send_500ms_cache_queue_ptr = NULL;
    void * total_pingpong_buf = NULL;
    char * copy_pingpong_buf[2] = {nullptr, nullptr};
}CACHE_10MS_INFO;

typedef struct asr_proc_stru {
    pthread_t _thr_process_id;
    void * send_500ms_queue_total_buf = NULL;
    SEND_500MS_QUEUE * send_asr_buf_free = NULL;
    SEND_500MS_QUEUE * send_asr_buf = NULL;
    SEND_500MS_QUEUE send_asr_nodes[SEND_500MS_BUF_NUM];
    volatile bool asr_send_server_active = false;
    volatile bool curl_start_session = true;
    volatile bool global_awake_voice_send = false;
    volatile bool awake_random_pingpong = true;
    volatile bool start_new_session = false;
    volatile int send_pakets_count = 0;
    MEMORY_STRU_URL_RECV recv_stru;
    CURL_ASR curl_asr_stru;
    CACHE_10MS_INFO cache_10ms_stru;
    SendLogProc * sendlogobj = NULL;
}ASR_PROC_STRU;

class InteractiveProcForASR {
 public:
    int Init();
    int DeInit();
    int GetAsrResult(char *in);
    int set_send_asr_info(char *send_buf, int send_buf_len, bool vadfg);
    int pause();

 private:
    int InitSendAsrCurlData();
    int DeInitSendAsrCurlData();
    int mutex_cond_init();
    int mutex_cond_deinit();
    int sendbuf_init();
    int sendbuf_deinit();
    int cache_pcm_buf_init();
    int cache_pcm_buf_deinit();
    bool start_thread();
    int delete_thread();
    int init_curl_session();
    int deinit_curl_session();
    void send_asr_buf_queue_push(SEND_500MS_QUEUE **head, SEND_500MS_QUEUE *node);
    SEND_500MS_QUEUE *send_asr_buf_queue_pop(SEND_500MS_QUEUE **head);
    static int asr_write_data(void *buffer, int size, int nmemb, void **stream);
    int SendDataToAsr(char *send_buf, int send_buf_len, int packets_num, bool vadflag);
    int cache_send_buf(char *cache_buf_tmp, char *read_buf, int &cache_num);
    static void *thread_read(void *arg);
    static void *thread_process(void *arg);
    static InteractiveProcForASR *static_obj;
    ASR_PROC_STRU asr_proc_info;
    ASR_MUTEX_AND_COND asr_mutex_cond;
    static volatile bool client_send_log_status;
};
#endif  // FRAME_WORK_SEND_ASR_H_
