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
#ifndef FRAME_WORK_FRAME_WORK_H_
#define FRAME_WORK_FRAME_WORK_H_
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <iostream>
#include <string>

#include "frame_work_common_def.h"
#include "play_audio.h"
#include "send_asr.h"
#include "tts_proc.h"
#include "clock_proc.h"
#include "client_log_dump.h"
#include "framework_volume_ctl.h"

#define DUMP_PCM_8_CHANNEL_FILE "eight_channel_dump.pcm"
#define CONFIG_FILE    "lefar.cfg"

#define FRAMEWORK_AUDIO_BUF_NUM 1
#define FRAMEWORK_DUMP_PCM_FILE_NUM 1

typedef struct frame_work_get_tts_info {
    char data[TTS_VOICE_LEN + 160 * 2 * 1 * 50] = {0};
    int copy_len = 0;
    volatile bool get_status = false;
}FRAME_WORK_GET_TTS_INFO;

enum FRAME_WORK_CHANNEL_INDEX {
    _RECORD_8_CHANNEL
};

enum DOA_TYPE {
    REAL_TIME_ANGLE_LED,
    AWAKE_ANGLE_LED
};

typedef struct framework_config_info {
    int doa_type = 0;
}FRAMEWORK_CONFIG_INFO;

typedef struct frame_work_module {
    volatile bool client_send_log_status = false;
    volatile bool real_wakeup_status = false;
    volatile bool global_wake_up_status = false;
    int awake_status = 0;
    int awake_count = 30;
    const int song_name_len = 100;
    const int artist_len = 50;
    char song_name[100] = {0};
    char artist[50] = {0};
    char tts_txt_buf[TTS_TXT_LEN] = {0};
    FRAME_WORK_GET_TTS_INFO tts_voice_buf;
    FRAMEWORK_CONFIG_INFO framework_config;
    Play_Audio_Data * play_obj = NULL;
    InteractiveProcForASR *asr_proc_obj = NULL;
    TtsProc *tts_proc_obj = NULL;
    ClockVoiceProc *clock_voice_obj = NULL;
    SendLogProc *send_log_to_serv_obj = NULL;
    FrameworkVolueProc *ctl_vol_obj = NULL;
}FRAME_WORK_MODULE;

class FrameWorkProc {
 public:
    int FrameWorkInit();
    int FrameWorkProcess();
    int FrameWorkDeInit();
    int SetKugouBinding(const char *userid, const char *token_id);
    int pause();
 private:
    int FileInfoInit();
    int AudioBuffInit();
    int OtherFunClassInit();
    int FileInfoDeInit();
    int AudioBuffDeInit();
    int OtherFunClassDeInit();
    int GetConfig();
    int AnalysisConfigInfo(const char *jchar);
    int Get_respeaker_data(char *audio_buf, const int &read_buf_len, bool &vadflag, bool &kwsflag, int16_t *angle);
    bool get_real_awake_status(bool &kwsflag);
    int analysis_asr_result(char *asr_res_buf);
    static void *get_tts_res(const char *pcm_buf, int pcm_buf_len, void *arg);
    FRAME_WORK_MODULE  framework_info_stru;
    char *AudioBuffPtr[FRAMEWORK_AUDIO_BUF_NUM] = {0};
    FILE *AudioFileFp[FRAMEWORK_DUMP_PCM_FILE_NUM] = {0};
};
#endif  // FRAME_WORK_FRAME_WORK_H_
