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
#ifndef FRAME_WORK_FRAMEWORK_VOLUME_CTL_H_
#define FRAME_WORK_FRAMEWORK_VOLUME_CTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <alsa/asoundlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/soundcard.h>
#include <pthread.h>

#ifdef __cplusplus
}
#endif

#include <iostream>

#include "frame_work_common_def.h"
#include "client_log_dump.h"

#define ALSA_MIXER_DEV  "hw:seeed8micvoicec"

//#define ALSA_MIXER_HEADPHONE_CTL "headphone volume"
#define ALSA_MIXER_HEADPHONE_CTL "Headphone"

//#define ALSA_MIXER_DIGITAL_CTL  "digital volume"

//#define ALSA_MIXER_SPEAKER_CTL "speaker volume"
#define ALSA_MIXER_SPEAKER_CTL "Speaker"

//#define ALSA_MIXER_DAC_VOLUME  "DAC volume"
#define ALSA_MIXER_DAC_VOLUME  "DAC"



typedef struct ctl_vol_mutex_cond {
    pthread_mutex_t ctl_vol_mutex;
    pthread_cond_t ctl_vol_cond;
}CTL_VOL_MUTEX_COND;

typedef struct framework_vol_info {
    pthread_t thread_id;
    volatile bool client_send_log_status = false;
    volatile bool ctl_vol_action = false;
    CTL_VOLUME_TYPE action_type = SET_VOLUME_DEFAULT;
    CTL_VOL_MUTEX_COND mutex_cond_stru;
    SendLogProc *sendlogobj = NULL;
}FRAMEWORK_VOL_INFO;

class FrameworkVolueProc {
 public:
    int Init();
    int DeInit();
    int SendSig(CTL_VOLUME_TYPE play_cation);
 private:
    int init_thread();
    int deinit_thread();
    int mutex_and_cond_init();
    int mutex_and_cond_deinit();
    static void *set_volume_thread(void *arg);
    int volume_get(void);
    int volume_set(int vol);
    int set_dev_volume_init();
    int volume_init(const char *alsa_mix_ctrl, snd_mixer_t *&alsa_mix_handle_p, snd_mixer_elem_t *&alsa_mix_elem_p);
    void volume_uninit(snd_mixer_t *&alsa_mix_handle_P);
    FRAMEWORK_VOL_INFO  ctl_vol_info;
};
#endif  // FRAME_WORK_FRAMEWORK_VOLUME_CTL_H_
