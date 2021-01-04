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
#ifndef FRAME_WORK_TTS_PROC_H_
#define FRAME_WORK_TTS_PROC_H_

#include <iostream>
#include <string>
#include <cstring>

#include "frame_work_common_def.h"
#include "client_log_dump.h"

#define CURL_RECV_TTS_VOICE_LEN  (160 * 2 * 1 * 2000)

#define TTS_POSTURL  "https://voice.lenovo.com/lasf/cloudtts"

typedef struct tts_curl_info {
    CURL *tts_curl = NULL;
    curl_slist *_headers = NULL;
}TTS_CURL_INFO;

typedef struct tts_callback_memory {
    char *memory = NULL;
    size_t size;
}TTS_CALLBACK_MEMORY;

typedef struct tts_mutex_and_cond {
    pthread_mutex_t txt_mutex;
    pthread_cond_t voice_cond;
}TTS_MUTEX_AND_COND;

typedef struct frame_work_callback_info {
    void *arg = NULL;
    TTS_RES get_tts_res = NULL;
}FRAME_WORK_CALLBACK_INFO;

typedef struct tts_info {
    pthread_t tts_voice_thread_id;
    volatile bool client_send_status = false;
    volatile bool tts_pause = false;
    volatile bool tts_voice_action = false;
    char tts_txt_record_buf[TTS_TXT_LEN] = {0};
    char tts_voice_2_channel_buf[TTS_VOICE_LEN] = {0};
    TTS_CALLBACK_MEMORY tts_url_callback_buf;
    TTS_CURL_INFO tts_curl_stru;
    TTS_MUTEX_AND_COND mutex_cond_stru;
    FRAME_WORK_CALLBACK_INFO frame_work_callback;
    SendLogProc *sendlogobj = NULL;
}TTS_INFO;

class TtsProc {
 private:
    int init_thread();
    int deinit_thread();
    int mutex_and_cond_init();
    int mutex_and_cond_deinit();
    int memory_init();
    int memory_deinit();
    std::string encode_to_url(std::string &str_source);
    int get_tts_voice(char *tts_curldata);
    static int write_ttsdata(void *buffer, int size, int nmemb, void *userp);
    static void * TtsProcess(void *arg);
    TTS_INFO tts_info_stru;
 public:
    TtsProc();
    ~TtsProc();
    int Init();
    int DeInit();
    int pause();
    int set_tts_info(const char *ttsTxt, int len, TTS_RES callback, void *arg);
};
#endif  // FRAME_WORK_TTS_PROC_H_
