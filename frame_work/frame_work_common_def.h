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
#ifndef FRAME_WORK_FRAME_WORK_COMMON_DEF_H_
#define FRAME_WORK_FRAME_WORK_COMMON_DEF_H_

#include "globaldef.h"
#include "cJSON.h"
#include "curl.h"

#define ASR_RESPONSE_MAX_MSG 1024 * 128
#define TTS_TXT_LEN  512  // tts txt  buf len
#define TTS_VOICE_LEN  (160 * 2 * 2 * 2000)      // tts voice  buf len

// Modify the macro, According to your key
#define LENOVOKEY_FRAMEWORK "lenovoKey: XXXXXXXXX"
#define SECRETKEY_FRAMEWORK "secretKey: XXXXXXXXX"
#define PLAY_LOCAL_MUSIC (1)  //  1:local 0:online  Currently only local music can be played. Online access to music environment is not available because it is a test environment.

typedef void * (*TTS_RES)(const char *pcm_buf, int pcm_buf_len, void *arg);

enum SYSTEM_STATUS {
    SYS_VOICE_RECOGNIZE,
    SYS_MUSIC_START,
    SYS_MUSIC_STOP,
    SYS_TTS,
    SYS_NO_HEAR,
    SYS_NET_ERR,
    SYS_NOT_UNDERSTAND,
    SYS_SET_CLOCK,
    SYS_SET_VOLUME,
    SYS_DEFAULT
};

typedef struct system_status_value {
    volatile SYSTEM_STATUS sys_type = SYS_DEFAULT;
    volatile SYSTEM_STATUS sys_old_type = SYS_DEFAULT;
}SYSTEM_STATUS_VALUE;

extern SYSTEM_STATUS_VALUE systemmod;

enum PLAY_ACTION {
    PLAY_MUSIC,
    STOP_MUSIC,
    PLAY_AWAKE,
    PLAY_NO_HEAR,
    PLAY_NET_ERR,
    PLAY_TTS,
    PLAY_NOT_UNDERSTAND,
    PLAY_CLOCK_TTS,
    PLAY_CLOCK_TTS_FILE,
    PLAY_CLOCK_RES_OK,
    PLAY_DEFAULT
};

typedef struct playthread_module_info {
    volatile bool play_thread_started = false;
    volatile bool play_thread_stop = false;
}PLAYTHREAD_MODULE_INFO;

extern PLAYTHREAD_MODULE_INFO playthreadmod;

enum ASR_RESULT {
    ASR_MUSIC_START,
    ASR_MUSIC_STOP,
    ASR_GET_TTS,
    ASR_NO_HEAR,
    ASR_NET_ERR,
    ASR_NOT_UNDERSTAND,
    ASR_SET_CLOCK,
    ASR_SET_VOL_UP,
    ASR_SET_VOL_DOWN,
    ASR_DEFAULT
};

typedef struct asrthread_module_info {
    volatile bool send_asr_thread_started = false;
    volatile bool send_asr_thread_stop = false;
    volatile bool global_asr_res = false;
    volatile ASR_RESULT  asr_mod = ASR_DEFAULT;
    volatile ASR_RESULT  old_asr_mod = ASR_DEFAULT;
}ASRTHREAD_MODULE_INFO;

extern ASRTHREAD_MODULE_INFO asrthreadmod;

typedef struct tts_proc_status {
    volatile bool tts_thread_start = false;
    volatile bool tts_thread_stop = false;
}TTS_PROC_STATUS;

extern TTS_PROC_STATUS ttsthreadmod;

typedef struct clock_voice_status {
    volatile bool clock_voice_thread_start = false;
    volatile bool clock_voice_thread_stop = false;
}CLOCK_VOICE_STATUS;

extern CLOCK_VOICE_STATUS clockvoicethreadmod;

enum CTL_VOLUME_TYPE {
    SET_VOLUME_UP,
    SET_VOLUME_DOWN,
    SET_VOLUME_DEFAULT
};

typedef struct volume_control_status {
    volatile bool volume_control_thread_start = false;
    volatile bool volume_control_thread_stop = false;
}VOLUME_CONTROL_STATUS;

extern VOLUME_CONTROL_STATUS volumecontrolthreadmod;

#endif  // FRAME_WORK_FRAME_WORK_COMMON_DEF_H_
