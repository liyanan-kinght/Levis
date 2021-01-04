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

#include "framework_volume_ctl.h"

#define __DEBUG__ 1
#define __INFO__ 1

#if __DEBUG__
#define DEBUG(format, ...) printf("FILE: " __FILE__ ", LINE: %d: " format "\n", __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif

#if __INFO__
#define INFO(format, ...) printf("FILE: " __FILE__ ", LINE: %d: " format "\n", __LINE__, ##__VA_ARGS__)
#else
#define INFO(format, ...)
#endif

VOLUME_CONTROL_STATUS volumecontrolthreadmod;

int FrameworkVolueProc::Init() {
#if __SEND_LOG_TO_SERVER__
    ctl_vol_info.sendlogobj = new SendLogProc();
    if (NULL == ctl_vol_info.sendlogobj) {
        printf("%s:%d:%s: ERROR [send log obj malloc err!]\n", __FILE__, __LINE__, __func__);
        ctl_vol_info.client_send_log_status = false;
    } else {
        ctl_vol_info.client_send_log_status = true;
    }

    if (-1 == ctl_vol_info.sendlogobj->Init()) {
        printf("%s:%d:%s: ERROR [send log Init err!]\n", __FILE__, __LINE__, __func__);
        ctl_vol_info.client_send_log_status = false;
    } else {
        ctl_vol_info.client_send_log_status = true;
    }
#endif
    if (-1 == set_dev_volume_init())
        return -1;

    mutex_and_cond_init();

    if (-1 == init_thread())
        return -1;

    printf("%s:%s:%d  set control volume Init success!\n", __FILE__, __func__, __LINE__);
    return 0;
}

int FrameworkVolueProc::DeInit() {
    pthread_mutex_lock(&ctl_vol_info.mutex_cond_stru.ctl_vol_mutex);
    pthread_cond_broadcast(&ctl_vol_info.mutex_cond_stru.ctl_vol_cond);
    pthread_mutex_unlock(&ctl_vol_info.mutex_cond_stru.ctl_vol_mutex);
    mutex_and_cond_deinit();
    if (-1 == deinit_thread())
        return -1;

    printf("%s:%s:%d  set control volume DeInit success!\n", __FILE__, __func__, __LINE__);

#if __SEND_LOG_TO_SERVER__
    if (NULL != ctl_vol_info.sendlogobj) {
        ctl_vol_info.sendlogobj->DeInit();
        delete ctl_vol_info.sendlogobj;
    }
#endif

    return 0;
}

int FrameworkVolueProc::set_dev_volume_init() {
    snd_mixer_t *alsa_mix_headphone_handle = NULL;
    //snd_mixer_t *alsa_mix_digital_handle = NULL;
    snd_mixer_t *alsa_mix_speaker_handle = NULL;
    snd_mixer_t *alsa_mix_dac_handle = NULL;

    snd_mixer_elem_t *alsa_mix_headphone_elem = NULL;
    //snd_mixer_elem_t *alsa_mix_digital_elem = NULL;
    snd_mixer_elem_t *alsa_mix_speaker_elem = NULL;
    snd_mixer_elem_t *alsa_mix_dac_elem = NULL;

    // Headphone volume 50
    if (-1 == volume_init(ALSA_MIXER_HEADPHONE_CTL, alsa_mix_headphone_handle, alsa_mix_headphone_elem))
        return -1;

    snd_mixer_selem_set_playback_volume(alsa_mix_headphone_elem, SND_MIXER_SCHN_FRONT_LEFT, 50);
    snd_mixer_selem_set_playback_volume(alsa_mix_headphone_elem, SND_MIXER_SCHN_FRONT_RIGHT, 50);
    volume_uninit(alsa_mix_headphone_handle);

    // speaker volume 50
    if (-1 == volume_init(ALSA_MIXER_SPEAKER_CTL, alsa_mix_speaker_handle, alsa_mix_speaker_elem))
        return -1;

    snd_mixer_selem_set_playback_volume(alsa_mix_speaker_elem, SND_MIXER_SCHN_FRONT_LEFT, 75);
    snd_mixer_selem_set_playback_volume(alsa_mix_speaker_elem, SND_MIXER_SCHN_FRONT_RIGHT, 75);
    volume_uninit(alsa_mix_speaker_handle);

    if (-1 == volume_init(ALSA_MIXER_DAC_VOLUME, alsa_mix_dac_handle, alsa_mix_dac_elem))
        return -1;

    snd_mixer_selem_set_playback_volume(alsa_mix_dac_elem, SND_MIXER_SCHN_FRONT_LEFT, 60);
    snd_mixer_selem_set_playback_volume(alsa_mix_dac_elem, SND_MIXER_SCHN_FRONT_RIGHT, 60);
    volume_uninit(alsa_mix_dac_handle);
#if 0
    // Digital volume 90
    if (-1 == volume_init(ALSA_MIXER_DIGITAL_CTL, alsa_mix_digital_handle, alsa_mix_digital_elem))
        return -1;
    snd_mixer_selem_set_playback_volume(alsa_mix_digital_elem, SND_MIXER_SCHN_FRONT_LEFT, 90);
    snd_mixer_selem_set_playback_volume(alsa_mix_digital_elem, SND_MIXER_SCHN_FRONT_RIGHT, 90);
    volume_uninit(alsa_mix_digital_handle);
    DEBUG("dev_volume: speaker==55 headphone==50 digital==90 dac_volume==60");

    if (ctl_vol_info.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "dev_volume: speaker==75 headphone==50  dac_volume==60 digital==90");
#endif
    DEBUG("dev_volume: speaker==55 headphone==50 dac_volume==90");

    if (ctl_vol_info.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "dev_volume: speaker==75 headphone==50 digital==90 dac_volume==60");

    return 0;
}

int FrameworkVolueProc::volume_init(const char *alsa_mix_ctrl, snd_mixer_t *&alsa_mix_handle_p, snd_mixer_elem_t *&alsa_mix_elem_p) {
    int alsa_mix_index = 0;
    snd_mixer_selem_id_t *alsa_mix_sid = NULL;

    snd_mixer_selem_id_alloca(&alsa_mix_sid);
    snd_mixer_selem_id_set_index(alsa_mix_sid, alsa_mix_index);
    snd_mixer_selem_id_set_name(alsa_mix_sid, alsa_mix_ctrl);

    if ((snd_mixer_open(&alsa_mix_handle_p, 0)) < 0) {
        DEBUG("Failed to open mixer");
        if (ctl_vol_info.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "Failed to open mixer");
        return -1;
    }

    if ((snd_mixer_attach(alsa_mix_handle_p, ALSA_MIXER_DEV)) < 0) {
        DEBUG("Failed to attach mixer");
        if (ctl_vol_info.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "Failed to attach mixer");
        return -1;
    }

    if ((snd_mixer_selem_register(alsa_mix_handle_p, NULL, NULL)) < 0) {
        DEBUG("Failed to register mixer element");
        if (ctl_vol_info.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "Failed to register mixer element");
        return -1;
    }

    if (snd_mixer_load(alsa_mix_handle_p) < 0) {
        DEBUG("Failed to load mixer element");
        if (ctl_vol_info.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "Failed to load mixer element");
        return -1;
    }

    alsa_mix_elem_p = snd_mixer_find_selem(alsa_mix_handle_p, alsa_mix_sid);
    if (!alsa_mix_elem_p) {
        DEBUG("Failed to find mixer element");
        if (ctl_vol_info.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "Failed to find mixer element");
        return -1;
    } else {
        printf("elem name:%s\n",snd_mixer_selem_get_name(alsa_mix_elem_p));
    }

    if (snd_mixer_selem_set_playback_volume_range(alsa_mix_elem_p, 0, 100) < 0) {
        DEBUG("Failed to set playback volume range");
        if (ctl_vol_info.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "Failed to set playback volume range");
        return -1;
    }

    return 0;
}

void FrameworkVolueProc::volume_uninit(snd_mixer_t * &alsa_mix_handle) {
    if (alsa_mix_handle) {
        snd_mixer_close(alsa_mix_handle);
    }
}

int FrameworkVolueProc::volume_get() {
    long int ll, lr, vol;
    snd_mixer_t *alsa_mix_speaker_handle = NULL;
    snd_mixer_elem_t *alsa_mix_speaker_elem = NULL;

    // speaker volume
    if (0 == volume_init(ALSA_MIXER_SPEAKER_CTL, alsa_mix_speaker_handle, alsa_mix_speaker_elem)) {
        snd_mixer_handle_events(alsa_mix_speaker_handle);
        if (snd_mixer_selem_is_playback_mono(alsa_mix_speaker_elem)) {
            snd_mixer_selem_get_playback_volume(alsa_mix_speaker_elem, SND_MIXER_SCHN_FRONT_LEFT, &vol);

            if (ctl_vol_info.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "volume_get vol %d", vol);
            } else {
                DEBUG("volume_get vol %d", vol);
            }
        } else {
            snd_mixer_selem_get_playback_volume(alsa_mix_speaker_elem, SND_MIXER_SCHN_FRONT_LEFT, &ll);
            snd_mixer_selem_get_playback_volume(alsa_mix_speaker_elem, SND_MIXER_SCHN_FRONT_RIGHT, &lr);

            vol = static_cast<int>((ll + lr) >> 1);

            if (ctl_vol_info.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "volume_get vol %d, ll %ld, lr %ld", vol, ll, lr);
            } else {
                DEBUG("volume_get vol %d, ll %ld, lr %ld", vol, ll, lr);
            }
        }

        volume_uninit(alsa_mix_speaker_handle);
    }

    return vol;
}

int FrameworkVolueProc::volume_set(int vol) {
    snd_mixer_t *alsa_mix_speaker_handle = NULL;
    snd_mixer_elem_t *alsa_mix_speaker_elem = NULL;

    if (vol > 100)
        vol = 100;
    if (vol < 0)
        vol = 0;

    // speaker channel
    if (0 == volume_init(ALSA_MIXER_SPEAKER_CTL, alsa_mix_speaker_handle, alsa_mix_speaker_elem)) {
        if (snd_mixer_selem_is_playback_mono(alsa_mix_speaker_elem)) {
            snd_mixer_selem_set_playback_volume(alsa_mix_speaker_elem, SND_MIXER_SCHN_FRONT_LEFT, vol);
        } else {
            snd_mixer_selem_set_playback_volume(alsa_mix_speaker_elem, SND_MIXER_SCHN_FRONT_LEFT, vol);
            snd_mixer_selem_set_playback_volume(alsa_mix_speaker_elem, SND_MIXER_SCHN_FRONT_RIGHT, vol);
        }

        volume_uninit(alsa_mix_speaker_handle);

        if (ctl_vol_info.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "volume_set vol %d", vol);
        } else {
            DEBUG("volume_set vol %d", vol);
        }
    }

    return 0;
}

int FrameworkVolueProc::init_thread() {
    int ret = 0;
    ret = pthread_create(&ctl_vol_info.thread_id, NULL, set_volume_thread, this);
    if (0 != ret) {
        printf("%s:%d:%s: ERROR[control volume thread create error!]\n", __FILE__, __LINE__, __func__);
        if (ctl_vol_info.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "ERROR[control volume thread create error!]");
        return -1;
    }

    return 0;
}

int FrameworkVolueProc::deinit_thread() {
    DEBUG("INTO THREAD DEINIT");
    pthread_join(ctl_vol_info.thread_id, NULL);

    if (ctl_vol_info.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "control volume thread deinit success!");

    printf("%s:%d:%s: control volume thread deinit success!\n", __FILE__, __LINE__, __func__);

    return 0;
}

int FrameworkVolueProc::mutex_and_cond_init() {
    pthread_mutex_init(&ctl_vol_info.mutex_cond_stru.ctl_vol_mutex, NULL);
    pthread_cond_init(&ctl_vol_info.mutex_cond_stru.ctl_vol_cond, NULL);

    if (ctl_vol_info.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "control volume  mutex and cond init success!");

    printf("%s:%d:%s: control volume  mutex and cond init success!\n", __FILE__, __LINE__, __func__);
    return 0;
}

int FrameworkVolueProc::mutex_and_cond_deinit() {
    pthread_mutex_destroy(&ctl_vol_info.mutex_cond_stru.ctl_vol_mutex);
    pthread_cond_destroy(&ctl_vol_info.mutex_cond_stru.ctl_vol_cond);

    if (ctl_vol_info.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(ctl_vol_info.sendlogobj, DEBUG, "control volume thread mutex and cond deinit success!");

    printf("%s:%d:%s: control volume thread mutex and cond deinit success!\n", __FILE__, __LINE__, __func__);

    return 0;
}

int FrameworkVolueProc::SendSig(CTL_VOLUME_TYPE play_cation) {
    ctl_vol_info.action_type = play_cation;
    ctl_vol_info.ctl_vol_action = true;
    pthread_cond_signal(&ctl_vol_info.mutex_cond_stru.ctl_vol_cond);
    return 0;
}

void * FrameworkVolueProc::set_volume_thread(void *arg) {
    FrameworkVolueProc * tmp_pt = static_cast<FrameworkVolueProc *>(arg);

    int vol = 0;

    pthread_mutex_lock(&tmp_pt->ctl_vol_info.mutex_cond_stru.ctl_vol_mutex);

    while (!volumecontrolthreadmod.volume_control_thread_stop) {
        while (!volumecontrolthreadmod.volume_control_thread_stop && false == tmp_pt->ctl_vol_info.ctl_vol_action) {
            if (tmp_pt->ctl_vol_info.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->ctl_vol_info.sendlogobj, DEBUG, "wait signal for control volume action !");
            } else {
                DEBUG("%s:wait signal for control volume action !", __func__);
            }

            pthread_cond_wait(&tmp_pt->ctl_vol_info.mutex_cond_stru.ctl_vol_cond, &tmp_pt->ctl_vol_info.mutex_cond_stru.ctl_vol_mutex);

            if (tmp_pt->ctl_vol_info.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->ctl_vol_info.sendlogobj, DEBUG, "wait signal for control volume action success!");
            } else {
                DEBUG("%s:wait signal for control volume action success!", __func__);
            }
        }

        vol = 0;

        switch (tmp_pt->ctl_vol_info.action_type) {
            case SET_VOLUME_UP:
                vol = tmp_pt->volume_get();
                tmp_pt->volume_set(vol + 10);

                if (tmp_pt->ctl_vol_info.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(tmp_pt->ctl_vol_info.sendlogobj, DEBUG, "set volume up!");
                } else {
                    DEBUG("%s:set volume up!", __func__);
                }

                if (SET_VOLUME_UP == tmp_pt->ctl_vol_info.action_type) {
                    tmp_pt->ctl_vol_info.ctl_vol_action = false;
                    tmp_pt->ctl_vol_info.action_type = SET_VOLUME_DEFAULT;
                }
                break;

            case SET_VOLUME_DOWN:
                vol = tmp_pt->volume_get();
                tmp_pt->volume_set(vol - 10);

                if (tmp_pt->ctl_vol_info.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(tmp_pt->ctl_vol_info.sendlogobj, DEBUG, "set volume down!");
                } else {
                    DEBUG("%s:set volume down!", __func__);
                }

                if (SET_VOLUME_DOWN == tmp_pt->ctl_vol_info.action_type) {
                    tmp_pt->ctl_vol_info.ctl_vol_action = false;
                    tmp_pt->ctl_vol_info.action_type = SET_VOLUME_DEFAULT;
                }
                break;

            default:
                tmp_pt->ctl_vol_info.ctl_vol_action = false;
                tmp_pt->ctl_vol_info.action_type = SET_VOLUME_DEFAULT;
                break;
        }
    }

    pthread_mutex_unlock(&tmp_pt->ctl_vol_info.mutex_cond_stru.ctl_vol_mutex);

    if (tmp_pt->ctl_vol_info.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(tmp_pt->ctl_vol_info.sendlogobj, DEBUG, "#### PLAYBACK THREAD END!");
    } else {
        printf("%s:%d:%s: #### PLAYBACK THREAD END! ####\n", __FILE__, __LINE__, __func__);
    }

    pthread_exit(NULL);
}
