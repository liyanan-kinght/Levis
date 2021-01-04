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
  * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  * OPOSSIBILITY OF SUCH DAMAGE.
  ***********************************************************************************************************
  */

#ifndef FRAME_WORK_CLOCK_PROC_H_
#define FRAME_WORK_CLOCK_PROC_H_

#include <iostream>
#include <string>
#include <cstring>

#include "frame_work_common_def.h"
#include "tts_proc.h"
#include "client_log_dump.h"

typedef struct clock_voice_info {
    pthread_t clock_voice_thr_id;
    volatile int count_time_num = 0;
    volatile int time_num = 0;
    volatile bool client_send_log_status = false;
    volatile bool get_clock_voice_status = false;
    volatile bool clock_setting = false;
    volatile bool tts_voice_action = false;
    char tts_txt_record_buf[TTS_TXT_LEN] = {0};
    char tts_voice_2_channel_buf[TTS_VOICE_LEN] = {0};
    volatile int clock_voice_channel_2_len = 0;
    TTS_CURL_INFO curl_stru;
    TTS_CALLBACK_MEMORY tts_cache_stru;
    TTS_MUTEX_AND_COND mutex_cond_stru;
    SendLogProc * sendlogobj = NULL;
}CLOCK_VOICE_INFO;

class ClockVoiceProc {
 private:
    int mutex_and_cond_init();
    int mutex_and_cond_deinit();
    int init_thread();
    int deinit_thread();
    int memory_init();
    int memory_deinit();
    static void * ClockTts(void * arg);
    static int write_ttsdata(void * buffer, int size, int nmemb, void *userp);
    std::string encode_to_url(std::string &str_source);
    int get_tts_voice(char *tts_curldata);
 public:
    CLOCK_VOICE_INFO clock_voice_stru;
    int Init();
    int DeInit();
    int AnalysisAsrResult(const  char * asr_result);
    int send_clock_sig();
    int time_count_proc();
    int play_select();
    int set_clock_voice_info_init();
    int set_clock_voice_info_deinit();
};
#endif  // FRAME_WORK_CLOCK_PROC_H_
