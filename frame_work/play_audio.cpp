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
#include "play_audio.h"

#define THREAD_SETPRIORITY 1

#define __DEBUG__ 1    // debug

#define __INFO__ 1     // Special key information for test development information

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

using std::string;

Play_Audio_Data * Play_Audio_Data::static_obj = nullptr;

PLAYTHREAD_MODULE_INFO  playthreadmod;

Play_Audio_Data::Play_Audio_Data() {
}

Play_Audio_Data::~Play_Audio_Data() {
}

int Play_Audio_Data::Init() {
#if __SEND_LOG_TO_SERVER__

    playback_info_stru.sendlogobj = new SendLogProc();
    if (NULL == playback_info_stru.sendlogobj) {
        printf("%s:%d:%s: ERROR [client send log obj malloc err!]\n", __FILE__, __LINE__, __func__);
        playback_info_stru.client_send_log_status = false;
    } else {
        playback_info_stru.client_send_log_status = true;
    }

    if (-1 == playback_info_stru.sendlogobj->Init()) {
        printf("%s:%d:%s: ERROR [send log Init err!]\n", __FILE__, __LINE__, __func__);
        playback_info_stru.client_send_log_status = false;
    } else {
        playback_info_stru.client_send_log_status = true;
    }

#endif

    mutex_and_cond_init();

    if (-1 == local_pcm_init()) {
        return -1;
    }

    if (-1 == play_snd_pcm_init()) {
        printf("%s:%d:%s: ERROR [play_snd_pcm_init err!]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [play_snd_pcm_init err!]\n", __func__);
        return -1;
    }

    if (-1 == thread_init()) {
        return -1;
    }

    printf("%s:%d:%s: init play audio success!\n", __FILE__, __LINE__, __func__);

    if (playback_info_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: init play audio success!\n", __func__);

    return 0;
}

int Play_Audio_Data::DeInit() {
    DEBUG("INTO PLAYBACK DEINIT");

    if (playback_info_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "INTO PLAYBACK DEINIT");

    pthread_mutex_lock(&playback_info_stru.mutex_and_cond_stru.playback_mutex);
    pthread_mutex_lock(&playback_info_stru.mutex_and_cond_stru.music_mutex);
    pthread_cond_broadcast(&playback_info_stru.mutex_and_cond_stru.playback_cond);
    pthread_cond_broadcast(&playback_info_stru.mutex_and_cond_stru.music_cond);
    pthread_mutex_unlock(&playback_info_stru.mutex_and_cond_stru.playback_mutex);
    pthread_mutex_unlock(&playback_info_stru.mutex_and_cond_stru.music_mutex);

    thread_deinit();
    local_pcm_deinit();
    mutex_and_cond_deinit();
    play_snd_pcm_deinit();

    #if __SEND_LOG_TO_SERVER__

    if (NULL != playback_info_stru.sendlogobj) {
        playback_info_stru.sendlogobj->DeInit();
        delete playback_info_stru.sendlogobj;
    }

    #endif

    printf("%s:%d:%s: deinit play audio success!\n", __FILE__, __LINE__, __func__);
    return 0;
}

int Play_Audio_Data::play_snd_pcm_init() {
    int rec = snd_pcm_open(&playback_info_stru.snd_pcm_stru._play_handle, "plughw:1, 0", SND_PCM_STREAM_PLAYBACK, 0);
    if (rec < 0) {
        printf("%s:%d:%s: ERROR [unable to open pcm device: %s]\n", __FILE__, __LINE__, __func__,  snd_strerror(rec));
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [unable to open pcm device: %s]\n", __func__,  snd_strerror(rec));
        return -1;
    }

    snd_pcm_hw_params_alloca(&playback_info_stru.snd_pcm_stru._playback_params);
    snd_pcm_hw_params_any(playback_info_stru.snd_pcm_stru._play_handle, playback_info_stru.snd_pcm_stru._playback_params);
    rec = snd_pcm_hw_params_set_access(playback_info_stru.snd_pcm_stru._play_handle, playback_info_stru.snd_pcm_stru._playback_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rec < 0) {
        printf("%s:%d:%s: ERROR [unable to open pcm device: %s]\n", __FILE__, __LINE__, __func__, snd_strerror(rec));
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [unable to open pcm device: %s]\n", __func__, snd_strerror(rec));
        return -1;
    }

    snd_pcm_hw_params_set_format(playback_info_stru.snd_pcm_stru._play_handle, playback_info_stru.snd_pcm_stru._playback_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(playback_info_stru.snd_pcm_stru._play_handle, playback_info_stru.snd_pcm_stru._playback_params, 1);

    int rc = snd_pcm_hw_params_set_rate_near(playback_info_stru.snd_pcm_stru._play_handle, playback_info_stru.snd_pcm_stru._playback_params, \
                                            &playback_info_stru.snd_pcm_stru._frequency, &playback_info_stru.snd_pcm_stru._playback_dir);
    if (rc < 0) {
        printf("%s:%d:%s: ERROR [set rate failed: %s]\n", __FILE__, __LINE__, __func__, snd_strerror(rc));
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [set rate failed: %s]\n", __func__, snd_strerror(rc));
        return -1;
    }

    snd_pcm_hw_params_set_period_size_near(playback_info_stru.snd_pcm_stru._play_handle, playback_info_stru.snd_pcm_stru._playback_params, \
                                        &playback_info_stru.snd_pcm_stru._playback_frames, &playback_info_stru.snd_pcm_stru._playback_dir);

    rc = snd_pcm_hw_params(playback_info_stru.snd_pcm_stru._play_handle, playback_info_stru.snd_pcm_stru._playback_params);
    if (rc < 0) {
        printf("%s:%d:%s: ERROR [unable to set hw parameters: %s]\n", __FILE__, __LINE__, __func__, snd_strerror(rc) );
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [unable to set hw parameters: %s]\n", __func__, snd_strerror(rc));
        return -1;
    }

    snd_pcm_hw_params_get_period_size(playback_info_stru.snd_pcm_stru._playback_params, &playback_info_stru.snd_pcm_stru._playback_frames, &playback_info_stru.snd_pcm_stru._playback_dir);
    snd_pcm_hw_params_get_channels(playback_info_stru.snd_pcm_stru._playback_params, &playback_info_stru.snd_pcm_stru._play_channels);
    snd_pcm_hw_params_get_rate(playback_info_stru.snd_pcm_stru._playback_params, &playback_info_stru.snd_pcm_stru._frequency, &playback_info_stru.snd_pcm_stru._playback_dir);

    playback_info_stru.snd_pcm_stru._playback_buf_size = playback_info_stru.snd_pcm_stru._playback_frames * playback_info_stru.snd_pcm_stru._datablock * playback_info_stru.snd_pcm_stru._play_channels;
    playback_info_stru.snd_pcm_stru._play_buffer = reinterpret_cast<char*>(malloc(playback_info_stru.snd_pcm_stru._playback_buf_size * sizeof(char)));
    if (NULL == playback_info_stru.snd_pcm_stru._play_buffer) {
        printf("%s:%d:%s: ERROR [malloc _play_buffer error!]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [malloc _play_buffer error!]\n", __func__);
        return -1;
    }

    memset(playback_info_stru.snd_pcm_stru._play_buffer, 0, playback_info_stru.snd_pcm_stru._playback_buf_size * sizeof(char));

    printf("%s:%d:%s: play_snd_pcm_init success!\n", __FILE__, __LINE__, __func__);

    if (playback_info_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: play_snd_pcm_init success!\n", __func__);
    return 0;
}

void Play_Audio_Data::play_snd_pcm_deinit() {
    // snd_pcm_hw_params_free(playback_info_stru.snd_pcm_stru._playback_params);
    snd_pcm_close(playback_info_stru.snd_pcm_stru._play_handle);

    if (playback_info_stru.snd_pcm_stru._play_buffer != NULL) {
        free(playback_info_stru.snd_pcm_stru._play_buffer);
        playback_info_stru.snd_pcm_stru._play_buffer = NULL;
    }
}

void Play_Audio_Data::mutex_and_cond_init() {
    pthread_mutex_init(&playback_info_stru.mutex_and_cond_stru.playback_mutex, NULL);
    pthread_cond_init(&playback_info_stru.mutex_and_cond_stru.playback_cond, NULL);

    pthread_mutex_init(&playback_info_stru.mutex_and_cond_stru.music_mutex, NULL);
    pthread_cond_init(&playback_info_stru.mutex_and_cond_stru.music_cond, NULL);

    pthread_mutex_init(&playback_info_stru.mutex_and_cond_stru.voice_mutex, NULL);
}

void Play_Audio_Data::mutex_and_cond_deinit() {
    pthread_mutex_destroy(&playback_info_stru.mutex_and_cond_stru.playback_mutex);
    pthread_cond_destroy(&playback_info_stru.mutex_and_cond_stru.playback_cond);

    pthread_mutex_destroy(&playback_info_stru.mutex_and_cond_stru.music_mutex);
    pthread_cond_destroy(&playback_info_stru.mutex_and_cond_stru.music_cond);

    pthread_mutex_destroy(&playback_info_stru.mutex_and_cond_stru.voice_mutex);

    printf("%s:%d:%s: playback mutex and cond deinit!\n", __FILE__, __LINE__, __func__);

    if (playback_info_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: playback mutex and cond deinit!\n", __func__);
}

int Play_Audio_Data::thread_init() {
#if THREAD_SETPRIORITY
    pthread_attr_t thread_attr;
    struct sched_param schedule_param;

    pthread_attr_init(&thread_attr);
    schedule_param.sched_priority = 30;
    pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);    // Set priority to take effect
    pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
    pthread_attr_setschedparam(&thread_attr, &schedule_param);

    int ret = pthread_create(&playback_info_stru.playback_thread_id, (const pthread_attr_t *) &thread_attr, playback_thread_proc, this);
#else
    int ret = pthread_create(&playback_info_stru.playback_thread_id, NULL, playback_thread_proc, this);
#endif
    if (0 != ret) {
        printf("%s:%d:%s: ERROR [playback thread create error!]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [playback thread create error!]\n", __func__);
        return -1;
    }

    #if THREAD_SETPRIORITY
    ret = pthread_create(&playback_info_stru.music_thread_id, (const pthread_attr_t *) &thread_attr, music_thread_proc, this);
    #else
    ret = pthread_create(&playback_info_stru.music_thread_id, NULL, music_thread_proc, this);
    #endif
    if (0 != ret) {
        printf("%s:%d:%s: ERROR [music thread create error!]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [music thread create error!]\n", __func__);
        return -1;
    }

    return 0;
}

void Play_Audio_Data::thread_deinit() {
    DEBUG("INTO PLAYBACK THREAD DEINIT");

    if (playback_info_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "INTO PLAYBACK THREAD DEINIT");

    pthread_join(playback_info_stru.playback_thread_id, NULL);
    pthread_join(playback_info_stru.music_thread_id, NULL);
    printf("%s:%d:%s:playback thread deinit!\n", __FILE__, __LINE__, __func__);

    if (playback_info_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:playback thread deinit!\n", __func__);
}

int Play_Audio_Data::local_pcm_init() {
    playback_info_stru.pcm_file[SYS_VOICE] = fopen("sys_voice.pcm", "rb");
    if (NULL == playback_info_stru.pcm_file[SYS_VOICE]) {
        printf("%s:%d:%s: ERROR [Fail To Open sys_voice.pcm !]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Fail To Open sys_voice.pcm !]\n", __func__);
        return -1;
    }

    playback_info_stru.pcm_file[AWAKE_VOICE] = fopen("btn_center_1_ding.pcm", "rb");
    if (NULL == playback_info_stru.pcm_file[AWAKE_VOICE]) {
        printf("%s:%d:%s: ERROR [Fail To Open awake_voice.pcm !]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Fail To Open awake_voice.pcm !]\n", __func__);
        return -1;
    }

    playback_info_stru.pcm_file[NO_HEAR_VOICE] = fopen("no_hear.pcm", "rb");
    if (NULL == playback_info_stru.pcm_file[NO_HEAR_VOICE]) {
        printf("%s:%d:%s: ERROR [Fail To Open no_hear.pcm !]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Fail To Open no_hear.pcm !]\n", __func__);
        return -1;
    }

    playback_info_stru.pcm_file[NOT_UNDERSTAND_VOICE] = fopen("not_understand.pcm", "rb");
    if (NULL == playback_info_stru.pcm_file[NOT_UNDERSTAND_VOICE]) {
        printf("%s:%d:%s: ERROR [Fail To Open not_understand.pcm !]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Fail To Open not_understand.pcm !]\n", __func__);
        return -1;
    }

    playback_info_stru.pcm_file[NET_ERR_VOICE] = fopen("net_err.pcm", "rb");
    if (NULL == playback_info_stru.pcm_file[NET_ERR_VOICE]) {
        printf("%s:%d:%s: ERROR [Fail To Open net_err.pcm !]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Fail To Open net_err.pcm !]\n", __func__);
        return -1;
    }

    playback_info_stru.pcm_file[MUSIC_LIST_NULL] = fopen("no_find_music.pcm", "rb");
    if (NULL == playback_info_stru.pcm_file[MUSIC_LIST_NULL]) {
        printf("%s:%d:%s: ERROR [Fail To Open no_find_music.pcm !]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Fail To Open no_find_music.pcm !]\n", __func__);
        return -1;
    }

    playback_info_stru.pcm_file[MUSIC_END_LIST_NULL] = fopen("music_list_end.pcm", "rb");
    if (NULL == playback_info_stru.pcm_file[MUSIC_END_LIST_NULL]) {
        printf("%s:%d:%s: ERROR [Fail To Open music_list_end.pcm !]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Fail To Open music_list_end.pcm !]\n", __func__);
        return -1;
    }

    playback_info_stru.pcm_file[GET_MUSIC_FAIL_VOICE] = fopen("get_music_fail.pcm", "rb");
    if (NULL == playback_info_stru.pcm_file[GET_MUSIC_FAIL_VOICE]) {
        printf("%s:%d:%s: ERROR [Fail To Open get_music_fail.pcm!]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Fail To Open get_music_fail.pcm!]\n", __func__);
        return -1;
    }

    playback_info_stru.pcm_file[GET_SONG_DADA_ERR_VOICE] = fopen("get_song_data_err.pcm", "rb");
    if (NULL == playback_info_stru.pcm_file[GET_SONG_DADA_ERR_VOICE]) {
        printf("%s:%d:%s: ERROR [Fail To Open get_song_data_err.pcm!]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Fail To Open get_song_data_err.pcm!]\n", __func__);
        return -1;
    }

    playback_info_stru.pcm_file[CLOCK_VOICE] = fopen("clock_voice.pcm", "rb");
    if (NULL == playback_info_stru.pcm_file[CLOCK_VOICE]) {
        printf("%s:%d:%s: ERROR [Fail To Open clock_voice.pcm!]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Fail To Open clock_voice.pcm!]\n", __func__);
        return -1;
    }

    playback_info_stru.pcm_file[CLOCK_SET_SUCESS_VOICE] = fopen("clock_set_sucess_voice.pcm", "rb");
    if (NULL == playback_info_stru.pcm_file[CLOCK_SET_SUCESS_VOICE]) {
        printf("%s:%d:%s: ERROR [Fail To Open clock_set_sucess_voice.pcm!]\n", __FILE__, __LINE__, __func__);
        if (playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Fail To Open clock_set_sucess_voice.pcm!]\n", __func__);
        return -1;
    }

    return 0;
}

void Play_Audio_Data::local_pcm_deinit() {
    if (playback_info_stru.pcm_file[SYS_VOICE]) {
        fclose(playback_info_stru.pcm_file[SYS_VOICE]);
        playback_info_stru.pcm_file[SYS_VOICE] = nullptr;
    }

    if (playback_info_stru.pcm_file[AWAKE_VOICE]) {
        fclose(playback_info_stru.pcm_file[AWAKE_VOICE]);
        playback_info_stru.pcm_file[AWAKE_VOICE] = nullptr;
    }

    if (playback_info_stru.pcm_file[NO_HEAR_VOICE]) {
        fclose(playback_info_stru.pcm_file[NO_HEAR_VOICE]);
        playback_info_stru.pcm_file[NO_HEAR_VOICE] = nullptr;
    }

    if (playback_info_stru.pcm_file[NOT_UNDERSTAND_VOICE]) {
        fclose(playback_info_stru.pcm_file[NOT_UNDERSTAND_VOICE]);
        playback_info_stru.pcm_file[NOT_UNDERSTAND_VOICE] = nullptr;
    }

    if (playback_info_stru.pcm_file[NET_ERR_VOICE]) {
        fclose(playback_info_stru.pcm_file[NET_ERR_VOICE]);
        playback_info_stru.pcm_file[NET_ERR_VOICE] = nullptr;
    }

    if (playback_info_stru.pcm_file[MUSIC_LIST_NULL]) {
        fclose(playback_info_stru.pcm_file[MUSIC_LIST_NULL]);
        playback_info_stru.pcm_file[MUSIC_LIST_NULL] = nullptr;
    }

    if (playback_info_stru.pcm_file[MUSIC_END_LIST_NULL]) {
        fclose(playback_info_stru.pcm_file[MUSIC_END_LIST_NULL]);
        playback_info_stru.pcm_file[MUSIC_END_LIST_NULL] = nullptr;
    }

    if (playback_info_stru.pcm_file[GET_MUSIC_FAIL_VOICE]) {
        fclose(playback_info_stru.pcm_file[GET_MUSIC_FAIL_VOICE]);
        playback_info_stru.pcm_file[GET_MUSIC_FAIL_VOICE] = nullptr;
    }

    if (playback_info_stru.pcm_file[GET_SONG_DADA_ERR_VOICE]) {
        fclose(playback_info_stru.pcm_file[GET_SONG_DADA_ERR_VOICE]);
        playback_info_stru.pcm_file[GET_SONG_DADA_ERR_VOICE] = nullptr;
    }

    if (playback_info_stru.pcm_file[CLOCK_VOICE]) {
        fclose(playback_info_stru.pcm_file[CLOCK_VOICE]);
        playback_info_stru.pcm_file[CLOCK_VOICE] = nullptr;
    }

    if (playback_info_stru.pcm_file[CLOCK_SET_SUCESS_VOICE]) {
        fclose(playback_info_stru.pcm_file[CLOCK_SET_SUCESS_VOICE]);
        playback_info_stru.pcm_file[CLOCK_SET_SUCESS_VOICE] = nullptr;
    }

    printf("%s:%d:%s: playback local voice pcm file close!\n", __FILE__, __LINE__, __func__);

    if (playback_info_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: playback local voice pcm file close!\n", __func__);
}

int Play_Audio_Data::pause() {
    // playback_info_stru.pause_status = true;
    // Pause is just to pause the playback, if there is no stop playback instruction, it will replay, add later
    // ...
    return 0;
}

int Play_Audio_Data::SendSig(PLAY_ACTION play_cation) {
    playback_info_stru.action_type = play_cation;
    playback_info_stru.play_thread_action = true;
    pthread_cond_signal(&playback_info_stru.mutex_and_cond_stru.playback_cond);
    return 0;
}

int Play_Audio_Data::set_tts_voice_info(char *data , int len) {
    memset(playback_info_stru.tts_voice_buf, 0, TTS_VOICE_LEN);
    memcpy(playback_info_stru.tts_voice_buf, data, len);
    playback_info_stru.tts_voice_len = len;
    return 0;
}

int Play_Audio_Data::set_clock_tts_voice_info(char *data , int len) {
    memset(playback_info_stru.clock_tts_voice_buf, 0, TTS_VOICE_LEN);
    memcpy(playback_info_stru.clock_tts_voice_buf + 160 * 2 * 1 * 50 , data, len);
    playback_info_stru.clock_tts_voice_len = len + 160 * 2 * 1 * 50;
    return 0;
}

int Play_Audio_Data::set_music_info(char *song, int song_len, char *artist, int artist_len) {
    memset(playback_info_stru.song_name, 0, playback_info_stru.song_name_len);
    memset(playback_info_stru.artist, 0, playback_info_stru.artist_len);
    sprintf(playback_info_stru.song_name, "%s", song);
    sprintf(playback_info_stru.artist, "%s", artist);

    return 0;
}

void * Play_Audio_Data::playback_thread_proc(void *arg) {
    printf("%s:%d:%s:#### PLAYBACK THREAD RUN! ####\n", __FILE__, __LINE__, __func__);

    Play_Audio_Data * tmp_pt = static_cast<Play_Audio_Data *>(arg);
    sleep(3);
    tmp_pt->play_pcm(tmp_pt->playback_info_stru.pcm_file[SYS_VOICE]);    // Local boot prompt

    if (playthreadmod.play_thread_stop) {
        printf("%s:%d:%s: #### PLAYBACK THREAD END! ####\n", __FILE__, __LINE__, __func__);
        pthread_exit(NULL);
    }

    tmp_pt->get_local_music();

    SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "#### PLAYBACK THREAD RUN! ####");

    pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.playback_mutex);
    while ( !playthreadmod.play_thread_stop ) {
        while (!playthreadmod.play_thread_stop && false == tmp_pt->playback_info_stru.play_thread_action) {
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:wait signal for play action !", __func__);
            } else {
                DEBUG("%s:wait signal for play action !", __func__);
            }
            pthread_cond_wait(&tmp_pt->playback_info_stru.mutex_and_cond_stru.playback_cond, &tmp_pt->playback_info_stru.mutex_and_cond_stru.playback_mutex);

            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:wait signal for play action success !", __func__);
            } else {
                DEBUG("%s:wait signal for play action success !", __func__);
            }
        }

        if (playthreadmod.play_thread_stop) {
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s: play_thread_stop is true!", __func__);
            } else {
                INFO("%s: play_thread_stop is true!", __func__);
            }
            break;
        }

        switch (tmp_pt->playback_info_stru.action_type) {
        case PLAY_AWAKE:
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s: 播放线程将要播放唤醒音频!", __func__);
            } else {
                DEBUG("%s: 播放线程将要播放唤醒音频!", __func__);
            }
            pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            tmp_pt->play_awake_pcm(tmp_pt->playback_info_stru.pcm_file[AWAKE_VOICE]);
            pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            if (PLAY_AWAKE == tmp_pt->playback_info_stru.action_type) {
                tmp_pt->playback_info_stru.play_thread_action = false;
                tmp_pt->playback_info_stru.action_type = PLAY_DEFAULT;
            }
            break;

        case PLAY_MUSIC:
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s: 播放线程将要播放音乐!", __func__);
            } else {
                DEBUG("%s: 播放线程将要播放音乐!", __func__);
            }
            tmp_pt->playback_info_stru.play_thread_action = false;
            tmp_pt->playback_info_stru.action_type = PLAY_DEFAULT;
            ++(tmp_pt->playback_info_stru.play_music_count);
            pthread_cond_signal(&tmp_pt->playback_info_stru.mutex_and_cond_stru.music_cond);    // Send a signal to the music thread to play music
            break;

        case STOP_MUSIC:
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s: 播放线程将停止播放音乐!", __func__);
            } else {
                DEBUG("%s: 播放线程将停止播放音乐!", __func__);
            }
            tmp_pt->playback_info_stru.play_thread_action = false;
            tmp_pt->playback_info_stru.action_type = PLAY_DEFAULT;
            tmp_pt->playback_info_stru.play_music_count = 0;
            // Play the music thread to restore the initial state, this later added
            // ...
            break;

        case PLAY_NO_HEAR:
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:播放线程将要播放没听清提示音!", __func__);
            } else {
                DEBUG("%s:播放线程将要播放没听清提示音!", __func__);
            }
            pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            tmp_pt->play_pcm(tmp_pt->playback_info_stru.pcm_file[NO_HEAR_VOICE]);
            pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            if (PLAY_NO_HEAR == tmp_pt->playback_info_stru.action_type) {
                tmp_pt->playback_info_stru.play_thread_action = false;
                tmp_pt->playback_info_stru.action_type = PLAY_DEFAULT;
            }
            break;

        case PLAY_NOT_UNDERSTAND:
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:播放线程将要播放没听懂提示音!", __func__);
            } else {
                DEBUG("%s:播放线程将要播放没听懂提示音!", __func__);
            }
            pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            tmp_pt->play_pcm(tmp_pt->playback_info_stru.pcm_file[NOT_UNDERSTAND_VOICE]);
            pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            if (PLAY_NOT_UNDERSTAND == tmp_pt->playback_info_stru.action_type) {
                tmp_pt->playback_info_stru.play_thread_action = false;
                tmp_pt->playback_info_stru.action_type = PLAY_DEFAULT;
            }
            break;

        case PLAY_NET_ERR:
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:播放线程将要播放net ERROR提示音!", __func__);
            } else {
                DEBUG("%s:播放线程将要播放net ERROR提示音!", __func__);
            }
            pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            tmp_pt->play_pcm(tmp_pt->playback_info_stru.pcm_file[NET_ERR_VOICE]);
            pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            if (PLAY_NET_ERR == tmp_pt->playback_info_stru.action_type) {
                tmp_pt->playback_info_stru.play_thread_action = false;
                tmp_pt->playback_info_stru.action_type = PLAY_DEFAULT;
            }
            break;

        case PLAY_TTS:
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:播放线程将要播放TTS!", __func__);
            } else {
                DEBUG("%s:播放线程将要播放TTS!", __func__);
            }
            pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            tmp_pt->play_pcm(tmp_pt->playback_info_stru.tts_voice_buf, tmp_pt->playback_info_stru.tts_voice_len);
            pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            if (PLAY_TTS == tmp_pt->playback_info_stru.action_type) {
                tmp_pt->playback_info_stru.play_thread_action = false;
                tmp_pt->playback_info_stru.action_type = PLAY_DEFAULT;
            }
            break;

        case PLAY_CLOCK_TTS:
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:播放线程将要播放clock voice TTS!", __func__);
            } else {
                DEBUG("%s:播放线程将要播放clock voice TTS!", __func__);
            }
            pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            tmp_pt->play_pcm(tmp_pt->playback_info_stru.clock_tts_voice_buf, tmp_pt->playback_info_stru.clock_tts_voice_len);    // TTS_VOICE_LEN);
            pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            if (PLAY_CLOCK_TTS == tmp_pt->playback_info_stru.action_type) {
                tmp_pt->playback_info_stru.play_thread_action = false;
                tmp_pt->playback_info_stru.action_type = PLAY_DEFAULT;
            }
            break;

        case PLAY_CLOCK_TTS_FILE:
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:播放线程将要播放clock voice file!", __func__);
            } else {
                DEBUG("%s:播放线程将要播放clock voice file!", __func__);
            }
            pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            tmp_pt->play_pcm(tmp_pt->playback_info_stru.pcm_file[CLOCK_VOICE]);
            pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            if (PLAY_CLOCK_TTS_FILE == tmp_pt->playback_info_stru.action_type) {
                tmp_pt->playback_info_stru.play_thread_action = false;
                tmp_pt->playback_info_stru.action_type = PLAY_DEFAULT;
            }
            break;

        case PLAY_CLOCK_RES_OK:
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:播放线程将要播放clock voice ok!", __func__);
            } else {
                DEBUG("%s:播放线程将要播放clock voice ok!", __func__);
            }
            pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            tmp_pt->play_pcm(tmp_pt->playback_info_stru.pcm_file[CLOCK_SET_SUCESS_VOICE]);
            pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            if (PLAY_CLOCK_RES_OK == tmp_pt->playback_info_stru.action_type) {
                tmp_pt->playback_info_stru.play_thread_action = false;
                tmp_pt->playback_info_stru.action_type = PLAY_DEFAULT;
            }
            break;

        default:
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:播放线程 default!", __func__);
            } else {
                DEBUG("%s:播放线程 default!", __func__);
            }
            tmp_pt->playback_info_stru.play_thread_action = false;
            tmp_pt->playback_info_stru.action_type = PLAY_DEFAULT;
            break;
        }
    }
    pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.playback_mutex);

    if (tmp_pt->playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "#### PLAYBACK THREAD END! ####");
    } else {
        printf("%s:%d:%s: #### PLAYBACK THREAD END! ####\n", __FILE__, __LINE__, __func__);
    }
    pthread_exit(NULL);
}

void * Play_Audio_Data::music_thread_proc(void *arg) {
    printf("%s:%d:%s:#### MUSIC THREAD RUN! ####\n", __FILE__, __LINE__, __func__);

    Play_Audio_Data * tmp_pt = static_cast<Play_Audio_Data *>(arg);
    static_obj = tmp_pt;

    if (playthreadmod.play_thread_stop) {
        printf("%s:%d:%s: #### MUSIC THREAD END! ####\n", __FILE__, __LINE__, __func__);
        if (tmp_pt->playback_info_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "#### MUSIC THREAD END! ####");
        pthread_exit(NULL);
    }

    SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "#### MUSIC THREAD RUN! ####");

    pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.music_mutex);
    while ( !playthreadmod.play_thread_stop ) {
        while (!playthreadmod.play_thread_stop && !tmp_pt->playback_info_stru.play_music_count) {
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:wait signal for play music!", __func__);
            } else {
                DEBUG("%s:wait signal for play music!", __func__);
            }
            pthread_cond_wait(&tmp_pt->playback_info_stru.mutex_and_cond_stru.music_cond, &tmp_pt->playback_info_stru.mutex_and_cond_stru.music_mutex);

            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:wait signal for play music success !", __func__);
            } else {
                DEBUG("%s:wait signal for play music success !", __func__);
            }
        }

        if (playthreadmod.play_thread_stop) {
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:play_thread_stop is true", __func__);
            } else {
                INFO("%s:play_thread_stop is true", __func__);
            }
            break;
        }

        if (tmp_pt->playback_info_stru.play_music_count > 0) {
            --(tmp_pt->playback_info_stru.play_music_count);
        } else {
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s: play_music_count <= 0\n", __func__);
            } else {
                printf("%s:%d:%s: play_music_count <= 0\n", __FILE__, __LINE__, __func__);
            }
            continue;
        }

        // 1. After waking up, this function should stop immediately, and let it play a wake-up tone
        // 2. If it does not wake up, play a network problem tone
        int ret_net_music = tmp_pt->net_music_proc();
        if (-1 == ret_net_music) {
            if (tmp_pt->playback_info_stru.music_net_err && SYS_MUSIC_START == systemmod.sys_type) {
                tmp_pt->play_pcm(tmp_pt->playback_info_stru.pcm_file[NET_ERR_VOICE]);
                if (tmp_pt->playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:net music play err , net err!", __func__);
                } else {
                    DEBUG("%s:net music play err , net err!", __func__);
                }
            } else {
                if (tmp_pt->playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:net music proc func end!", __func__);
                } else {
                    DEBUG("%s:net music proc func end!", __func__);
                }
            }

            systemmod.sys_old_type = SYS_MUSIC_STOP;
            tmp_pt->playback_info_stru.music_net_err = false;
        } else if (NO_MUSIC_LIST == ret_net_music) {
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:没有找到你要听的音乐！", __func__);
            } else {
                DEBUG("%s:没有找到你要听的音乐！", __func__);
            }
            pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            tmp_pt->play_pcm(tmp_pt->playback_info_stru.pcm_file[MUSIC_LIST_NULL]);
            pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            systemmod.sys_old_type = SYS_MUSIC_STOP;
            tmp_pt->playback_info_stru.music_net_err = false;
        } else if (MUSIC_LIST_END == ret_net_music) {
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:歌曲列表已为空，停止播放!", __func__);
            } else {
                DEBUG("%s:歌曲列表已为空，停止播放!", __func__);
            }
            pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            tmp_pt->play_pcm(tmp_pt->playback_info_stru.pcm_file[MUSIC_END_LIST_NULL]);
            pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            systemmod.sys_old_type = SYS_MUSIC_STOP;
            tmp_pt->playback_info_stru.music_net_err = false;
        } else if (GET_MUSIC_FAIL == ret_net_music) {
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:获取音乐失败！", __func__);
            } else {
                DEBUG("%s:获取音乐失败！", __func__);
            }
            pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            tmp_pt->play_pcm(tmp_pt->playback_info_stru.pcm_file[GET_MUSIC_FAIL_VOICE]);
            pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            systemmod.sys_old_type = SYS_MUSIC_STOP;
            tmp_pt->playback_info_stru.music_net_err = false;
        } else if (GET_SONG_DATA_ERR == ret_net_music) {
            if (tmp_pt->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:获取音乐数据失败！", __func__);
            } else {
                DEBUG("%s:获取音乐数据失败！", __func__);
            }
            pthread_mutex_lock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            tmp_pt->play_pcm(tmp_pt->playback_info_stru.pcm_file[GET_SONG_DADA_ERR_VOICE]);
            pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.voice_mutex);
            systemmod.sys_old_type = SYS_MUSIC_STOP;
            tmp_pt->playback_info_stru.music_net_err = false;
        }

        if (tmp_pt->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "%s:music play func end!!", __func__);
        } else {
            DEBUG("%s:music play func end!!", __func__);
        }
    }

    pthread_mutex_unlock(&tmp_pt->playback_info_stru.mutex_and_cond_stru.music_mutex);

    if (tmp_pt->playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(tmp_pt->playback_info_stru.sendlogobj, DEBUG, "#### MUSIC THREAD END! ####");
    } else {
        printf("%s:%d:%s: #### MUSIC THREAD END! ####\n", __FILE__, __LINE__, __func__);
    }
    pthread_exit(NULL);
}

int Play_Audio_Data::play_awake_pcm(FILE * voice_file) {
    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "INTO PLAY AWAKE PCM FILE!", __func__);
    } else {
        DEBUG("INTO PLAY AWAKE PCM FILE!", __func__);
    }

    fseek(voice_file, 0, SEEK_SET);

    snd_pcm_prepare(playback_info_stru.snd_pcm_stru._play_handle);

    while (!feof(voice_file)) {
        int _ret = 0, read_len = 0;
        bzero(playback_info_stru.snd_pcm_stru._play_buffer, playback_info_stru.snd_pcm_stru._playback_buf_size);
        read_len = fread(playback_info_stru.snd_pcm_stru._play_buffer, 1, playback_info_stru.snd_pcm_stru._playback_buf_size, voice_file);
        _ret = snd_pcm_writei(playback_info_stru.snd_pcm_stru._play_handle, playback_info_stru.snd_pcm_stru._play_buffer, playback_info_stru.snd_pcm_stru._playback_frames);
        if (_ret == -EPIPE) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: play_pcm underrun occurred\n", __func__);
            } else {
                printf("%s:%d:%s: play_pcm underrun occurred\n", __FILE__, __LINE__, __func__);
            }
            snd_pcm_prepare(playback_info_stru.snd_pcm_stru._play_handle);
        } else if (_ret < 0) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: play_pcm error from write: %s\n", __func__ , snd_strerror(_ret));
            } else {
                printf("%s:%d:%s: play_pcm error from write: %s\n", __FILE__, __LINE__, __func__ , snd_strerror(_ret));
            }
        } else if (_ret != static_cast<int>(playback_info_stru.snd_pcm_stru._playback_frames)) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: play_pcm short write, write %d _playback_frames\n", __func__, _ret);
            } else {
                printf("%s:%d:%s: play_pcm short write, write %d _playback_frames\n", __FILE__, __LINE__, __func__, _ret);
            }
        }

        if ( feof(voice_file) ) {
            break;
        }

        if (playthreadmod.play_thread_stop) {
            break;
        }
    }

    fseek(voice_file, 0, SEEK_SET);

    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:PLAY AWAKE PCM FILE END!", __func__);
    } else {
        DEBUG("%s:PLAY AWAKE PCM FILE END!", __func__);
    }
    return 0;
}

int Play_Audio_Data::play_pcm(FILE * voice_file) {
    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "INTO PLAY PCM FILE!", __func__);
    } else {
        DEBUG("INTO PLAY PCM FILE!", __func__);
    }
    fseek(voice_file, 0, SEEK_SET);

    snd_pcm_prepare(playback_info_stru.snd_pcm_stru._play_handle);

    while (SYS_VOICE_RECOGNIZE != systemmod.sys_type) {
        int _ret = 0, read_len = 0;
        bzero(playback_info_stru.snd_pcm_stru._play_buffer, playback_info_stru.snd_pcm_stru._playback_buf_size);
        read_len = fread(playback_info_stru.snd_pcm_stru._play_buffer, 1, playback_info_stru.snd_pcm_stru._playback_buf_size, voice_file);

        _ret = snd_pcm_writei(playback_info_stru.snd_pcm_stru._play_handle, playback_info_stru.snd_pcm_stru._play_buffer, playback_info_stru.snd_pcm_stru._playback_frames);
        if (_ret == -EPIPE) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: play_pcm underrun occurred\n", __func__);
            } else {
                printf("%s:%d:%s: play_pcm underrun occurred\n", __FILE__, __LINE__, __func__);
            }
            snd_pcm_prepare(playback_info_stru.snd_pcm_stru._play_handle);
        } else if (_ret < 0) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: play_pcm error from write: %s\n", __func__ , snd_strerror(_ret));
            } else {
                printf("%s:%d:%s: play_pcm error from write: %s\n", __FILE__, __LINE__, __func__ , snd_strerror(_ret));
            }
        } else if (_ret != static_cast<int>(playback_info_stru.snd_pcm_stru._playback_frames)) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: play_pcm short write, write %d _playback_frames\n", __func__, _ret);
            } else {
                printf("%s:%d:%s: play_pcm short write, write %d _playback_frames\n", __FILE__, __LINE__, __func__, _ret);
            }
        }

        if ( feof(voice_file) ) {
            break;
        }

        if (playthreadmod.play_thread_stop) {
            break;
        }
    }

    fseek(voice_file, 0, SEEK_SET);

    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:PLAY PCM FILE END!", __func__);
    } else {
        DEBUG("%s:PLAY PCM FILE END!", __func__);
    }
    return 0;
}

int Play_Audio_Data::play_pcm(const char * voice_buf, int len) {
    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:INTO PLAY MEMORY BUF VOICE", __func__);
    } else {
        DEBUG("%s:INTO PLAY MEMORY BUF VOICE", __func__);
    }
    int tmp_len = 0 , ret = 0;

    snd_pcm_prepare(playback_info_stru.snd_pcm_stru._play_handle);

    while (SYS_VOICE_RECOGNIZE != systemmod.sys_type) {
        bzero(playback_info_stru.snd_pcm_stru._play_buffer, playback_info_stru.snd_pcm_stru._playback_buf_size);
        if (len - tmp_len >= playback_info_stru.snd_pcm_stru._playback_buf_size) {
            memcpy(playback_info_stru.snd_pcm_stru._play_buffer, voice_buf + tmp_len, playback_info_stru.snd_pcm_stru._playback_buf_size);
        } else if (0 < len - tmp_len && len - tmp_len < playback_info_stru.snd_pcm_stru._playback_buf_size) {
            memcpy(playback_info_stru.snd_pcm_stru._play_buffer, voice_buf + tmp_len, len-tmp_len);
        } else if (0 > len-tmp_len) {
            break;
        }

        ret = snd_pcm_writei(playback_info_stru.snd_pcm_stru._play_handle, playback_info_stru.snd_pcm_stru._play_buffer, playback_info_stru.snd_pcm_stru._playback_frames);
        if (ret == -EPIPE) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: play_pcm underrun occurred\n", __func__);
            } else {
                printf("%s:%d:%s: play_pcm underrun occurred\n", __FILE__, __LINE__, __func__);
            }
            snd_pcm_prepare(playback_info_stru.snd_pcm_stru._play_handle);
        } else if (ret < 0) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: play_pcm error from write: %s\n", __func__ , snd_strerror(ret));
            } else {
                printf("%s:%d:%s: play_pcm error from write: %s\n", __FILE__, __LINE__, __func__ , snd_strerror(ret));
            }
        } else if (ret != static_cast<int>(playback_info_stru.snd_pcm_stru._playback_frames)) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: play_pcm short write, write %d _playback_frames\n", __func__, ret);
            } else {
                printf("%s:%d:%s: play_pcm short write, write %d _playback_frames\n", __FILE__, __LINE__, __func__, ret);
            }
        }

        if (len-tmp_len <= 0) {
            break;
        }

        tmp_len += playback_info_stru.snd_pcm_stru._playback_buf_size;
    }

    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:PLAY PCM BUF END!", __func__);
    } else {
        DEBUG("%s:PLAY PCM BUF END!", __func__);
    }
    return 0;
}


int Play_Audio_Data::get_local_music() {
    string filePath = LOCAL_MUSIC_PATH;
    getFiles(filePath, local_music_files);
    return 0;
}

void Play_Audio_Data::getFiles(std::string &path, std::vector<std::string>& files) {
#if 1
    DIR *dir = nullptr;
    struct dirent *ptr = nullptr;
    char base[1000];
    if ((dir=opendir(path.c_str())) == nullptr) {
        perror("Open dir error...");
        exit(1);
    }

    string p;

    while ((ptr=readdir(dir)) != nullptr) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
            continue;

        } else if (ptr->d_type == 8) {  //  file
            files.push_back(p.assign(path).append("/").append(ptr->d_name));
        } else if (ptr->d_type == 10) {  //  link file
            files.push_back(p.assign(path).append("/").append(ptr->d_name));
        } else if (ptr->d_type == 4) {   //  dir
            memset(base, '\0', sizeof(base));
            strcpy(base, path.c_str());
            strcat(base, "/");
            strcat(base, ptr->d_name);
            string tmp_dir = base;
            getFiles(tmp_dir, files);
        }
    }
    closedir(dir);
#else

    long hFile = 0;
    struct _finddata_t fileinfo;
    string p;

    if ((hFile = findfirst(p.assign(path).append("/*.mp3").c_str(), &fileinfo)) != -1) {
        do {
            if ((fileinfo.attrib & _A_SUBDIR)) {
                if (strcmp(fileinfo.name, ".") != 0  &&  strcmp(fileinfo.name, "..") != 0)
                    getFiles(p.assign(path).append("/").append(fileinfo.name), files);
            } else {
                files.push_back(p.assign(path).append("/").append(fileinfo.name));
            }
        } while (findnext(hFile, &fileinfo) == 0);
        findclose(hFile);
    }
#endif

    return;
}

//  Play the song prompt
void Play_Audio_Data::music_info_voice() {
    pthread_mutex_lock(&playback_info_stru.mutex_and_cond_stru.voice_mutex);
    play_pcm(playback_info_stru.tts_voice_buf, playback_info_stru.tts_voice_len);
    pthread_mutex_unlock(&playback_info_stru.mutex_and_cond_stru.voice_mutex);
}

int Play_Audio_Data::check_local_music(std::string &s) {
    std::string song_name = playback_info_stru.song_name;
    std::string artist_name = playback_info_stru.artist;

    if (song_name.empty()) {
        if (artist_name.empty()) {
        } else {
            string::size_type position_artist;
            position_artist = s.find(playback_info_stru.artist);
            if (position_artist != s.npos) {
                //  nothing
            } else {
                return -1;
            }
        }
    } else {
        string::size_type position_song;
        position_song = s.find(playback_info_stru.song_name);
        if (position_song != s.npos) {
            if (artist_name.empty()) {
                //  nothing
            } else {
                string::size_type position_artist;
                position_artist = s.find(playback_info_stru.artist);
                if (position_artist != s.npos) {
                    //  nothing
                } else {
                    return -1;
                }
            }
        } else {
            return -1;
        }
    }

    printf("Find music, begin play music\n");
    return 0;
}

int Play_Audio_Data::get_local_music_data(std::string &music_file) {
    FILE *data = fopen(music_file.c_str(), "rb");
    if (nullptr == data) {
        return -1;
    }

    fseek(data, 0L, SEEK_END);
    int size = ftell(data);
    fseek(data, 0L, SEEK_SET);
    int count_num = 0;

    while (size != count_num) {
        int tmp_num = fread(playback_info_stru.url_recv_stru.recv_buf, 1, size, data);
        count_num += tmp_num;
    }

    playback_info_stru.url_recv_stru.recv_len = count_num;
    fclose(data);

    return 0;
}

int Play_Audio_Data::play_local_music() {
#if 0
    while (1) {
        string file;
        std::cout << "######## local music #########  music num:" << local_music_files.size() << std::endl;
        for (int i = 0; i < local_music_files.size(); ++i) {
            file = local_music_files[i];
            std::cout << file << "num:" << i << std::endl;
            if (-1 == get_local_music_data(file)) {
                continue;
            }

            play_music(playback_info_stru.url_recv_stru.recv_buf, playback_info_stru.url_recv_stru.recv_len);
        }
    }

#else
    string file;
    for (int i = 0; i < local_music_files.size(); ++i) {
        if (!check_local_music(local_music_files[i])) {
            file = local_music_files[i];
            break;
        }
    }

    if (file.empty()) {
        return GET_MUSIC_FAIL;
    } else {
        if (-1 == get_local_music_data(file)) {
            return GET_MUSIC_FAIL;
        }
        music_info_voice();
        play_music(playback_info_stru.url_recv_stru.recv_buf, playback_info_stru.url_recv_stru.recv_len);
    }
#endif

    return 0;
}

//  Play network song processing function
int Play_Audio_Data::net_music_proc() {
    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: into play network music!", __func__);
    } else {
        DEBUG("%s: into play network music!", __func__);
    }

#if PLAY_LOCAL_MUSIC
    if (SYS_MUSIC_START == systemmod.sys_type) {
        int ret = play_local_music();
        return ret;
    }
#else
    if (SYS_MUSIC_START == systemmod.sys_type) {
        long ret_code = 0;
        char token_id[33] = {0} , music_list[512] = {0};

        CURLcode ret = CURL_LAST;
        if (SYS_MUSIC_START == systemmod.sys_type) {
            ret = Loading_Cms_Token(TOKEN_URL, 1000, ret_code);
            if (SYS_MUSIC_START == systemmod.sys_type) {
                if (CURLE_OK == ret && 200 == ret_code) {
                    if (-1 == analysis_token_id(token_id)) {   //  Analysis result
                        return GET_MUSIC_FAIL;
                    }
                } else {
                    if (playback_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [ret:%d ret_code:%d]\n", __func__, ret, ret_code);
                        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Loading_Cms_Token fail! error:%s]\n", __func__, curl_easy_strerror(ret));
                    } else {
                        printf("%s:%d:%s: ERROR [ret:%d ret_code:%d]\n", __FILE__, __LINE__, __func__, ret, ret_code);
                        printf("%s:%d:%s: ERROR [Loading_Cms_Token fail! error:%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret));
                    }

                    if (CURLE_OK == ret) {  //  Non-network problem is the failure to obtain
                        return GET_MUSIC_FAIL;
                    } else {  // Internet problem
                        playback_info_stru.music_net_err = true;
                        return -1;
                    }
                }
            }
        }

        if (playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:演唱者：%s 歌曲名：%s ", __func__, playback_info_stru.artist, playback_info_stru.song_name);
        } else {
            INFO("%s:演唱者：%s 歌曲名：%s ", __func__, playback_info_stru.artist, playback_info_stru.song_name);
        }
        string musiclist_urlstr;    // Music list url
        sprintf(music_list, MUSIC_LIST_URL_ARG, playback_info_stru.song_name, playback_info_stru.artist);

        if (playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:音乐列表:%s", __func__, music_list);
        } else {
            INFO("%s:音乐列表:%s", __func__, music_list);
        }
        string music_list_str = music_list;
        musiclist_urlstr = urlencode(music_list_str);

        if (SYS_MUSIC_START == systemmod.sys_type) {
            ret = Get_MusicList(musiclist_urlstr, 300, token_id, ret_code);
            if (SYS_MUSIC_START == systemmod.sys_type) {
                if (CURLE_OK == ret && 200 == ret_code) {
                    // Analysis result
                    int ret_analysis = analysis_music_info(token_id);
                    if (-1 == ret_analysis) {
                        return -1;
                    } else if (NO_MUSIC_LIST == ret_analysis) {
                        return NO_MUSIC_LIST;
                    } else if (MUSIC_LIST_END == ret_analysis) {
                        return MUSIC_LIST_END;
                    } else if (GET_MUSIC_FAIL == ret_analysis) {
                        return GET_MUSIC_FAIL;
                    } else if (GET_SONG_DATA_ERR == ret_analysis) {
                        return GET_SONG_DATA_ERR;
                    }
                } else {
                    if (playback_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [ret:%d ret_code:%d]\n", __func__, ret, ret_code);
                        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Get_musiclist fail! error:%s]\n", __func__, curl_easy_strerror(ret));
                    } else {
                        printf("%s:%d:%s: ERROR [ret:%d ret_code:%d]\n", __FILE__, __LINE__, __func__, ret, ret_code);
                        printf("%s:%d:%s: ERROR [Get_musiclist fail! error:%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret));
                    }

                    if (CURLE_OK == ret) {  //  Non-network problem is the failure to obtain
                        return GET_MUSIC_FAIL;
                    } else {  //  Internet problem
                        playback_info_stru.music_net_err = true;
                        return -1;
                    }
                }
            }
        }
    }
#endif
    return 0;
}

int Play_Audio_Data::write_loading_data(void* buffer, int size, int nmemb, void **stream) {
    memcpy(static_obj->playback_info_stru.url_recv_stru.recv_buf + static_obj->playback_info_stru.url_recv_stru.recv_len , buffer, (size_t)(size * nmemb));
    static_obj->playback_info_stru.url_recv_stru.recv_len += (size_t)(size * nmemb);

    return size * nmemb;
}

CURLcode Play_Audio_Data::App_Binding_Token(const std::string & strUrl, int nTimeout, const char *token_nm) {
    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "#############into binding kugou id proc! ###########\n");
    } else {
        printf("#############into binding kugou id proc! ###########\n");
    }
    long ret_code = 0;
    systemmod.sys_type = SYS_MUSIC_START;
    CURLcode ret_curl_load = Loading_Cms_Token(TOKEN_URL, 1000, ret_code);
    systemmod.sys_type = SYS_DEFAULT;
    if (CURLE_OK != ret_curl_load) {
        ret_code = 0;
        char token_num[40] = {0};
        sprintf(token_num, "token:%s", token_nm);

        /* Apply for libcurl session */
        CURLcode ret_curl = CURL_LAST;
        CURL* pCURL = NULL;
        pCURL = curl_easy_init();
        if ( pCURL == NULL ) {
            fprintf(stderr, "Cannot use the curl functions!\n");
            return ret_curl;
        }

        struct curl_slist* headers = NULL;
        // Splicing http request header
        headers = curl_slist_append(headers, token_num);
        headers = curl_slist_append(headers, "XXX");  //  "mid:xxxx");
        // Set handle value
        curl_easy_setopt(pCURL, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(pCURL, CURLOPT_URL, strUrl.c_str());
        curl_easy_setopt(pCURL, CURLOPT_VERBOSE, 0L);    // Print message information
        curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
        curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(pCURL, CURLOPT_SSL_VERIFYPEER, false);    // Do not verify the CA certificate
        curl_easy_setopt(pCURL, CURLOPT_SSL_VERIFYHOST, 0L);

        playback_info_stru.url_recv_stru.recv_len = 0;
        memset(playback_info_stru.url_recv_stru.recv_buf, 0, sizeof(playback_info_stru.url_recv_stru.recv_buf));

        curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, write_loading_data);

        ret_curl = curl_easy_perform(pCURL);
        if (CURLE_OK != ret_curl) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [curl_easy_perform is fail! error:%s]\n", __func__, curl_easy_strerror(ret_curl));
            } else {
                printf("%s:%d:%s: ERROR [curl_easy_perform is fail! error:%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl));
            }
            curl_easy_cleanup(pCURL);
            return ret_curl;
        }

        ret_curl = curl_easy_getinfo(pCURL, CURLINFO_RESPONSE_CODE, &ret_code);
        if (CURLE_OK != ret_curl) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [curl_easy_getinfo is fail! error:%s]\n", __func__, curl_easy_strerror(ret_curl));
            } else {
                printf("%s:%d:%s: ERROR [curl_easy_getinfo is fail! error:%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl));
            }
            curl_easy_cleanup(pCURL);
            return ret_curl;
        }

        curl_easy_cleanup(pCURL);

        if (playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "############# binding kugou id success! ###########\n");
        } else {
            printf("############# binding kugou id success! ###########\n");
        }
        return ret_curl;
    }
    return ret_curl_load;
}

CURLcode Play_Audio_Data::Loading_Cms_Token(const char * strUrl, int timeout, long &ret_code) {
    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:into Loading_Cms_Token!", __func__);
    } else {
        DEBUG("%s:into Loading_Cms_Token!", __func__);
    }
    CURLcode ret_curl = CURL_LAST;
    if (SYS_MUSIC_START == systemmod.sys_type) {
        CURL* pCURL = curl_easy_init();
        if ( pCURL == NULL ) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Cannot use the curl functions!]\n", __func__);
            } else {
                printf("%s:%d:%s: ERROR [Cannot use the curl functions!]\n", __FILE__, __LINE__, __func__);
            }
            return CURLE_FAILED_INIT;
        }

        curl_easy_setopt(pCURL, CURLOPT_URL, strUrl);
        curl_easy_setopt(pCURL, CURLOPT_HEADER, 0L);
        curl_easy_setopt(pCURL, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
        // curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
        // curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 1L);
        // curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, loading_fp_res);
        curl_easy_setopt(pCURL, CURLOPT_SSL_VERIFYPEER, false);    // Do not verify the CA certificate
        curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_LIMIT, 1L);    // Minimum speed limit, if less than 1 byte per second
        curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_TIME, 10L);    // The minimum time limit, if the transmission speed is lower than this speed, it will be suspended after 10s
        curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, 10L);    // Receive data timeout setting, if the data is not received within 10 seconds, exit directly
        curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 10L);    // Link timeout, 10 seconds

        playback_info_stru.url_recv_stru.recv_len = 0;
        memset(playback_info_stru.url_recv_stru.recv_buf, 0, sizeof(playback_info_stru.url_recv_stru.recv_buf));

        curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, write_loading_data);

        if (SYS_MUSIC_START == systemmod.sys_type) {
            ret_curl = curl_easy_perform(pCURL);
            if (CURLE_OK != ret_curl) {
                if (playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [curl_easy_perform is fail! error:%s]\n", __func__, curl_easy_strerror(ret_curl));
                } else {
                    printf("%s:%d:%s: ERROR [curl_easy_perform is fail! error:%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl));
                }
                curl_easy_cleanup(pCURL);
                return ret_curl;
            }
        }

        if (SYS_MUSIC_START == systemmod.sys_type) {
            ret_curl = curl_easy_getinfo(pCURL, CURLINFO_RESPONSE_CODE, &ret_code);
            if (CURLE_OK != ret_curl) {
                if (playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [curl_easy_getinfo is fail! error:%s]\n", __func__, curl_easy_strerror(ret_curl));
                } else {
                    printf("%s:%d:%s: ERROR [curl_easy_getinfo is fail! error:%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl));
                }
            }
        }

        curl_easy_cleanup(pCURL);
    }

    return ret_curl;
}

int Play_Audio_Data::analysis_token_id(char * token_id) {
    if (SYS_MUSIC_START == systemmod.sys_type) {
        cJSON *ret_parse = cJSON_Parse(playback_info_stru.url_recv_stru.recv_buf);
        if (!ret_parse) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [parse url_song_recv.recv_buf error! error:%s]\n", __func__, cJSON_GetErrorPtr());
            } else {
                printf("%s:%d:%s: ERROR [parse url_song_recv.recv_buf error! error:%s]\n", __FILE__, __LINE__, __func__, cJSON_GetErrorPtr());
            }
            return -1;
        }

        cJSON * ret_status = cJSON_GetObjectItem(ret_parse, "status");
        if (NULL != ret_status) {
            char status_fg[10] = {0};
            sprintf(status_fg, "%s", ret_status->valuestring);
            if (0 == strcmp(status_fg, "Y")) {
                cJSON * ret_res = cJSON_GetObjectItem(ret_parse, "res");
                if (NULL != ret_res) {
                    cJSON * ret_token = cJSON_GetObjectItem(ret_res, "token");
                    if (NULL != ret_token) {
                        sprintf(token_id, "%s", ret_token->valuestring);
                    } else {
                        if (playback_info_stru.client_send_log_status) {
                            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [get cms token is NULL!]\n", __func__);
                        } else {
                            printf("%s:%d:%s: ERROR [get cms token is NULL!]\n", __FILE__, __LINE__, __func__);
                        }
                        cJSON_Delete(ret_parse);
                        return -1;
                    }
                } else {
                    if (playback_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [res is NULL]\n", __func__);
                    } else {
                        printf("%s:%d:%s: ERROR [res is NULL]\n", __FILE__, __LINE__, __func__);
                    }
                    cJSON_Delete(ret_parse);
                    return -1;
                }
            } else {
                char msg_err[512] = {0};
                cJSON * ret_msg = cJSON_GetObjectItem(ret_parse, "msg");
                sprintf(msg_err, "%s", ret_msg->valuestring);

                if (playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Loading status_fg:%s , msg_err:%s]\n", __func__, status_fg, msg_err);
                } else {
                    printf("%s:%d:%s: ERROR [Loading status_fg:%s , msg_err:%s]\n", __FILE__, __LINE__, __func__, status_fg, msg_err);
                }
                cJSON_Delete(ret_parse);
                return -1;
            }
        } else {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [status is NULL!]\n", __func__);
            } else {
                printf("%s:%d:%s: ERROR [status is NULL!]\n", __FILE__, __LINE__, __func__);
            }
            cJSON_Delete(ret_parse);
            return -1;
        }

        cJSON_Delete(ret_parse);
    }

    return 0;
}

CURLcode Play_Audio_Data::Get_MusicList(const std::string & strUrl, int nTimeout, char *token_nm, long &ret_code) {
    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: into Get Music List!", __func__);
    } else {
        DEBUG("%s: into Get Music List!", __func__);
    }
    CURLcode ret_curl = CURL_LAST;
    if (SYS_MUSIC_START == systemmod.sys_type) {
        char _token_num[40] = {0};
        sprintf(_token_num, "token:%s", token_nm);

        CURL* pCURL = curl_easy_init();
        if ( pCURL == NULL ) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Cannot use the curl functions!]\n", __func__);
            } else {
                printf("%s:%d:%s: ERROR [Cannot use the curl functions!]\n", __FILE__, __LINE__, __func__);
            }
            return CURLE_FAILED_INIT;
        }

        curl_slist * _cms_headers = NULL;
        // https
        _cms_headers = curl_slist_append(_cms_headers, _token_num);
        _cms_headers = curl_slist_append(_cms_headers, "XXX");  //  "mid:xxx");

        curl_easy_setopt(pCURL, CURLOPT_HTTPHEADER, _cms_headers);
        curl_easy_setopt(pCURL, CURLOPT_URL, strUrl.c_str());
        curl_easy_setopt(pCURL, CURLOPT_HEADER, 0L);
        curl_easy_setopt(pCURL, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
        curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 1L);
        // curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, musiclist_fp_res);
        curl_easy_setopt(pCURL, CURLOPT_SSL_VERIFYPEER, false);    // Do not verify the CA certificate
        curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_LIMIT, 1L);    // Minimum speed limit, if less than 1 byte per second
        curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_TIME, 10L);    // The minimum time limit, if the transmission speed is lower than this speed, it will be suspended after 10ss
        curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, 10L);    // Receive data timeout setting, if the data is not received within 10 seconds, exit directly
        curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 10L);    // Link timeout, 10 seconds

        playback_info_stru.url_recv_stru.recv_len = 0;
        memset(playback_info_stru.url_recv_stru.recv_buf, 0, sizeof(playback_info_stru.url_recv_stru.recv_buf));
        curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, write_loading_data);

        if (SYS_MUSIC_START == systemmod.sys_type) {
            ret_curl = curl_easy_perform(pCURL);
            if (CURLE_OK != ret_curl) {
                if (playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [curl_easy_perform is fail! error:%s]\n", __func__, curl_easy_strerror(ret_curl));
                } else {
                    printf("%s:%d:%s: ERROR [curl_easy_perform is fail! error:%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl));
                }
                curl_slist_free_all(_cms_headers);
                curl_easy_cleanup(pCURL);
                return ret_curl;
            }
        }

        if (SYS_MUSIC_START == systemmod.sys_type) {
            ret_curl = curl_easy_getinfo(pCURL, CURLINFO_RESPONSE_CODE, &ret_code);
            if (CURLE_OK != ret_curl) {
                if (playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [curl_easy_getinfo is fail! error:%s]\n", __func__, curl_easy_strerror(ret_curl));
                } else {
                    printf("%s:%d:%s: ERROR [curl_easy_getinfo is fail! error:%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl));
                }
            }
        }

        curl_slist_free_all(_cms_headers);
        curl_easy_cleanup(pCURL);
    }

    return ret_curl;
}

int Play_Audio_Data::analysis_music_info(char * token_id) {
    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: into analysis music list result!", __func__);
    } else {
        DEBUG("%s: into analysis music list result!", __func__);
    }
    if (SYS_MUSIC_START == systemmod.sys_type) {
        cJSON *ret_parse = cJSON_Parse(playback_info_stru.url_recv_stru.recv_buf);
        if (!ret_parse) {
            printf("%s:%d:%s: ERROR [recv_music_list buf fail! error:%s]\n", __FILE__, __LINE__, __func__, cJSON_GetErrorPtr());
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [recv_music_list buf fail! error:%s]\n", __func__, cJSON_GetErrorPtr());
            } else {
                printf("%s:%d:%s: ERROR [recv_music_list buf fail! error:%s]\n", __FILE__, __LINE__, __func__, cJSON_GetErrorPtr());
            }
            return GET_MUSIC_FAIL;
        } else {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:%s", __func__, playback_info_stru.url_recv_stru.recv_buf);
            } else {
                INFO("%s:%s", __func__, playback_info_stru.url_recv_stru.recv_buf);
            }
            cJSON * ret_status = cJSON_GetObjectItem(ret_parse, "status");
            if (NULL != ret_status) {
                char status_fg[10] = {0};
                sprintf(status_fg, "%s", ret_status->valuestring);
                if (strcmp(status_fg, "Y") != 0) {
                    char msg_err[512] = {0};
                    cJSON * ret_msg = cJSON_GetObjectItem(ret_parse, "msg");
                    sprintf(msg_err, "%s", ret_msg->valuestring);

                    if (playback_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Get_MusicList status_fg:%s , msg_err%s]\n", __func__, status_fg, msg_err);
                    } else {
                        printf("%s:%d:%s: ERROR [Get_MusicList status_fg:%s , msg_err%s]\n", __FILE__, __LINE__, __func__, status_fg, msg_err);
                    }
                    cJSON_Delete(ret_parse);
                    return GET_MUSIC_FAIL;
                } else {
                    cJSON * ret_res = cJSON_GetObjectItem(ret_parse, "res");
                    if (NULL != ret_res) {
                        cJSON * ret_rows = cJSON_GetObjectItem(ret_res, "rows");
                        if (NULL != ret_rows) {
                            cJSON * rows_list = ret_rows->child;  // rows_list is NULL Exit and return, the play does not find the song list you said
                            if (NULL == rows_list) {
                                if (playback_info_stru.client_send_log_status) {
                                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:song list is NULL!", __func__);
                                } else {
                                    INFO("%s:song list is NULL!", __func__);
                                }
                                cJSON_Delete(ret_parse);
                                return NO_MUSIC_LIST;
                            } else {
                                int ret_music_proc_fun = music_proc(rows_list, token_id);
                                if (-1 == ret_music_proc_fun) {
                                    cJSON_Delete(ret_parse);
                                    return -1;
                                } else if (MUSIC_LIST_END == ret_music_proc_fun) {
                                    cJSON_Delete(ret_parse);
                                    return MUSIC_LIST_END;
                                } else if (GET_MUSIC_FAIL == ret_music_proc_fun) {
                                    cJSON_Delete(ret_parse);
                                    return GET_MUSIC_FAIL;
                                } else if (GET_SONG_DATA_ERR == ret_music_proc_fun) {
                                    cJSON_Delete(ret_parse);
                                    return GET_SONG_DATA_ERR;
                                }
                            }
                        } else {
                            if (playback_info_stru.client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Get_MusicList rows is NULL!]\n",  __func__);
                            } else {
                                printf("%s:%d:%s: ERROR [Get_MusicList rows is NULL!]\n", __FILE__, __LINE__, __func__);
                            }
                            cJSON_Delete(ret_parse);
                            return GET_MUSIC_FAIL;
                        }
                    } else {
                        if (playback_info_stru.client_send_log_status) {
                            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Get_MusicList res is NULL!]\n", __func__);
                        } else {
                            printf("%s:%d:%s: ERROR [Get_MusicList res is NULL!]\n", __FILE__, __LINE__, __func__);
                        }
                        cJSON_Delete(ret_parse);
                        return GET_MUSIC_FAIL;
                    }
                }
            } else {
                if (playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [music_list status is NULL!]\n", __func__);
                } else {
                    printf("%s:%d:%s: ERROR [music_list status is NULL!]\n", __FILE__, __LINE__, __func__);
                }
                cJSON_Delete(ret_parse);
                return GET_MUSIC_FAIL;
            }
        }

        cJSON_Delete(ret_parse);
    }
    return 0;
}

int Play_Audio_Data::music_proc(cJSON * &rows_list, char * &token_id) {
    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: into play music proc!", __func__);
    } else {
        DEBUG("%s: into play music proc!", __func__);
    }
    bool get_music_data_status = false;
    int  get_music_data_count = 0;

    if (SYS_MUSIC_START == systemmod.sys_type) {
        pthread_mutex_lock(&playback_info_stru.mutex_and_cond_stru.voice_mutex);
        play_pcm(playback_info_stru.tts_voice_buf, playback_info_stru.tts_voice_len);
        pthread_mutex_unlock(&playback_info_stru.mutex_and_cond_stru.voice_mutex);

        size_t len = 0;
        char params[256] = {0} , status_fg[10] = {0}, msg_err[512] = {0};
        char * songId = NULL, *albumId = NULL , *songUrl = NULL;
        string song_url_str;
        cJSON * bf_rows_list = rows_list;
        long ret_code = 0;
        CURLcode ret;

        cJSON * ret_music = NULL , *ret_status = NULL , *ret_msg = NULL , *ret_res = NULL;

        while (NULL != rows_list && SYS_MUSIC_START == systemmod.sys_type) {
            songId = NULL;
            albumId = NULL;
            songUrl = NULL;
            song_url_str.clear();
            memset(params, 0, sizeof(params));

            songId = cJSON_GetObjectItem(rows_list, "songId")->valuestring;
            albumId = cJSON_GetObjectItem(rows_list, "albumId")->valuestring;

            if ( strlen(albumId) == 0 ) {
                sprintf(params, "XXX%s", songId);  // "https://xxx.xxx.com.cn/musicSearch/audio/getSong?songId=%s&albumId=0&cpId=kg", songId);
            } else {
                sprintf(params, "XXX%s %s", songId, albumId);  //  "https://xxx.xxx.com.cn/musicSearch/audio/getSong?songId=%s&albumId=%s&cpId=kg", songId, albumId);
            }

            song_url_str = params;

            if (SYS_MUSIC_START == systemmod.sys_type) {
                ret = Get_SongUrl(song_url_str, 300, token_id, ret_code);
                if (SYS_MUSIC_START == systemmod.sys_type) {
                    if (CURLE_OK == ret && 200 == ret_code) {
                        if (NULL != ret_music) {
                            cJSON_Delete(ret_music);
                            ret_music = NULL;
                        }

                        ret_music = cJSON_Parse(playback_info_stru.url_recv_stru.recv_buf);
                        if (!ret_music) {
                            if (playback_info_stru.client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Get_SongUrl rec_buf error:%s]\n", __func__, cJSON_GetErrorPtr());
                            } else {
                                printf("%s:%d:%s: ERROR [Get_SongUrl rec_buf error:%s]\n", __FILE__, __LINE__, __func__, cJSON_GetErrorPtr());
                            }
                            return GET_MUSIC_FAIL;
                        } else {
                            ret_status = cJSON_GetObjectItem(ret_music, "status");
                            if (NULL != ret_status) {
                                memset(status_fg, 0, sizeof(status_fg));
                                sprintf(status_fg, "%s", ret_status->valuestring);
                                if (strcmp(status_fg, "Y") != 0) {
                                    ret_msg = cJSON_GetObjectItem(ret_music, "msg");
                                    memset(msg_err, 0, sizeof(msg_err));
                                    sprintf(msg_err, "%s", ret_msg->valuestring);

                                    if (playback_info_stru.client_send_log_status) {
                                        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Get_SongUrl status_fg:%s , msg_err:%s]\n", __func__, status_fg, msg_err);
                                    } else {
                                        printf("%s:%d:%s: ERROR [Get_SongUrl status_fg:%s , msg_err:%s]\n", __FILE__, __LINE__, __func__, status_fg, msg_err);
                                    }
                                    cJSON_Delete(ret_music);

                                    return GET_MUSIC_FAIL;
                                } else {
                                    ret_res = cJSON_GetObjectItem(ret_music, "res");
                                    if (NULL != ret_res) {
                                        songUrl = cJSON_GetObjectItem(ret_res, "songUrl")->valuestring;

                                        if (playback_info_stru.client_send_log_status) {
                                            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:songurl:%s", __func__, songUrl);
                                        } else {
                                            INFO("%s:songurl:%s", __func__, songUrl);
                                        }
                                    } else {
                                        if (playback_info_stru.client_send_log_status) {
                                            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [songurl is NULL!]\n", __func__);
                                        } else {
                                            printf("%s:%d:%s: ERROR [songurl is NULL!]\n", __FILE__, __LINE__, __func__);
                                        }
                                        cJSON_Delete(ret_music);
                                        return GET_MUSIC_FAIL;
                                    }
                                }
                            } else {
                                if (playback_info_stru.client_send_log_status) {
                                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Get_SongUrl recv_buf status is NULL!]\n", __func__);
                                } else {
                                    printf("%s:%d:%s: ERROR [Get_SongUrl recv_buf status is NULL!]\n", __FILE__, __LINE__, __func__);
                                }
                                cJSON_Delete(ret_music);
                                return GET_MUSIC_FAIL;
                            }
                        }
                    } else {
                        if (playback_info_stru.client_send_log_status) {
                            SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Get_SongUrl fail! error:%s ret_code:%d]\n", __func__, curl_easy_strerror(ret), ret_code);
                        } else {
                            printf("%s:%d:%s: ERROR [Get_SongUrl fail! error:%s ret_code:%d]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret), ret_code);
                        }
                        if (CURLE_OK == ret) {
                            return GET_MUSIC_FAIL;
                        } else {
                            playback_info_stru.music_net_err = true;
                            return -1;
                        }
                    }
                }
            }

            // Get song data according to songUrl
            if (SYS_MUSIC_START == systemmod.sys_type) {
                get_music_data_count = 0;    //  Number of requests
                get_music_data_status = true;    //  true:Play song
                char Redirect_url[1024] = {0};
                char in_url[1024] = {0};
                bool get_redirect_url_status = false;
                while (get_music_data_status && SYS_MUSIC_START == systemmod.sys_type) {
                    ++get_music_data_count;

                    if (playback_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:请求歌曲数据的次数：%d\n", __func__, get_music_data_count);
                    } else {
                        printf("%s:%d:%s:请求歌曲数据的次数：%d\n", __FILE__, __LINE__, __func__, get_music_data_count);
                    }
                    ret = Get_SongData(songUrl, 300, ret_code, Redirect_url, sizeof(Redirect_url), get_redirect_url_status);
                    if (SYS_MUSIC_START == systemmod.sys_type) {
                        if (CURLE_OK == ret && 200 == ret_code) {
                            if (0 == playback_info_stru.url_recv_stru.recv_len) {
                                if (playback_info_stru.client_send_log_status) {
                                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [SongData is NULL!]\n", __func__);
                                } else {
                                    printf("%s:%d:%s: ERROR [SongData is NULL!]\n", __FILE__, __LINE__, __func__);
                                }
                                cJSON_Delete(ret_music);
                                return GET_SONG_DATA_ERR;
                            }

                            get_music_data_status = false;
                            play_music(playback_info_stru.url_recv_stru.recv_buf, playback_info_stru.url_recv_stru.recv_len);
                            if (SYS_VOICE_RECOGNIZE == systemmod.sys_type || SYS_MUSIC_STOP == systemmod.sys_type || SYS_SET_CLOCK == systemmod.sys_type) {
                                cJSON_Delete(ret_music);
                                return 0;
                            }
                        } else {
                            if (playback_info_stru.client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Get_SongData fail! error:%s ret_code:%d]\n", __func__, curl_easy_strerror(ret), ret_code);
                            } else {
                                printf("%s:%d:%s: ERROR [Get_SongData fail! error:%s ret_code:%d]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret), ret_code);
                            }
                            if (get_redirect_url_status) {
                                memcpy(in_url, Redirect_url, sizeof(Redirect_url));
                                songUrl = in_url;

                                if (playback_info_stru.client_send_log_status) {
                                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "重新获取的url给songUrl赋值！");
                                } else {
                                    DEBUG("重新获取的url给songUrl赋值！");
                                }
                            }

                            if (2 <= get_music_data_count) {
                                get_music_data_status = false;

                                if (playback_info_stru.client_send_log_status) {
                                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:请求歌曲数据的次数：%d, 已经请求两次，结束歌曲数据的请求！\n", __func__, get_music_data_count);
                                } else {
                                    printf("%s:%d:%s:请求歌曲数据的次数：%d, 已经请求两次，结束歌曲数据的请求！\n", __FILE__, __LINE__, __func__, get_music_data_count);
                                }
                                cJSON_Delete(ret_music);
                                if (CURLE_OK == ret) {
                                    return GET_SONG_DATA_ERR;
                                } else {
                                    playback_info_stru.music_net_err = true;
                                    return -1;
                                }
                            }
                        }
                    } else {
                        get_music_data_status = false;
                    }
                }
            }

            rows_list = rows_list->next;
            if (NULL == rows_list) {
                if (SYS_MUSIC_START == systemmod.sys_type) {
                    if (playback_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:音乐列表播放结束!", __func__);
                    } else {
                        DEBUG("%s:音乐列表播放结束!", __func__);
                    }
                    systemmod.sys_old_type = SYS_MUSIC_STOP;
                }

                cJSON_Delete(ret_music);

                if (SYS_VOICE_RECOGNIZE == systemmod.sys_type || SYS_MUSIC_STOP == systemmod.sys_type) {
                    return 0;
                } else {
                    return MUSIC_LIST_END;
                }
            }

            // Loop song list
            /*
            rows_list = rows_list->next;
            if (NULL == rows_list)
            {
                rows_list = bf_rows_list;
            } 
            */ 
        }
    }
    return 0;
}

CURLcode Play_Audio_Data::Get_SongUrl(const std::string &strUrl, int nTimeout, char *token_nm, long &ret_code) {
    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: into Get Song URL!", __func__);
    } else {
        DEBUG("%s: into Get Song URL!", __func__);
    }

    CURLcode ret_curl = CURL_LAST;
    if (SYS_MUSIC_START == systemmod.sys_type) {
        char _token_num[40] = {0};
        sprintf(_token_num, "token:%s", token_nm);

        CURL* pCURL = curl_easy_init();
        if ( pCURL == NULL ) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Cannot use the curl functions!]\n", __func__);
            } else {
                printf("%s:%d:%s: ERROR [Cannot use the curl functions!]\n", __FILE__, __LINE__, __func__);
            }
            return CURLE_FAILED_INIT;
        }

        curl_slist * _cms_headers = NULL;

        _cms_headers = curl_slist_append(_cms_headers, _token_num);
        _cms_headers = curl_slist_append(_cms_headers, "XXX");  //  "mid:xxxx");

        curl_easy_setopt(pCURL, CURLOPT_HTTPHEADER, _cms_headers);
        curl_easy_setopt(pCURL, CURLOPT_URL, strUrl.c_str());
        curl_easy_setopt(pCURL, CURLOPT_HEADER, 0L);
        curl_easy_setopt(pCURL, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
        curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 1L);
        // curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, songurl_fp_res);
        curl_easy_setopt(pCURL, CURLOPT_SSL_VERIFYPEER, false);    // Do not verify the CA certificate
        curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_LIMIT, 1L);    // Minimum speed limit, if less than 1 byte per second
        curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_TIME, 10L);    // The minimum time limit, if the transmission speed is lower than this speed, it will be suspended after 10ss
        curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, 10L);    // Receive data timeout setting, if the data is not received within 10 seconds, exit directly
        curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 10L);    // Link timeout, 10 seconds

        playback_info_stru.url_recv_stru.recv_len = 0;
        memset(playback_info_stru.url_recv_stru.recv_buf, 0, sizeof(playback_info_stru.url_recv_stru.recv_buf));

        curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, write_loading_data);
        if (SYS_MUSIC_START == systemmod.sys_type) {
            ret_curl = curl_easy_perform(pCURL);
            if (CURLE_OK != ret_curl) {
                curl_slist_free_all(_cms_headers);
                curl_easy_cleanup(pCURL);

                if (playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [curl_easy_perform is fail! error:%s]\n", __func__, curl_easy_strerror(ret_curl));
                } else {
                    printf("%s:%d:%s: ERROR [curl_easy_perform is fail! error:%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl));
                }
                return ret_curl;
            }
        }

        if (SYS_MUSIC_START == systemmod.sys_type) {
            ret_curl = curl_easy_getinfo(pCURL, CURLINFO_RESPONSE_CODE, &ret_code);
            if (CURLE_OK != ret_curl) {
                if (playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:curl_easy_getinfo is fail! error:%s\n", __func__, curl_easy_strerror(ret_curl));
                } else {
                    printf("%s:%d:%s:curl_easy_getinfo is fail! error:%s\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl));
                }
            }
        }

        curl_slist_free_all(_cms_headers);
        curl_easy_cleanup(pCURL);
    }

    return ret_curl;
}

CURLcode Play_Audio_Data::Get_SongData(char * strUrl, int nTimeout, long &ret_code, char * Redirect_url, int Redirect_url_len, bool &get_redirect_url_status) {
    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:into Get Song mp3 Data!", __func__);
    } else {
        DEBUG("%s:into Get Song mp3 Data!", __func__);
    }

    CURLcode ret_curl;
    memset(Redirect_url, 0, Redirect_url_len);
    get_redirect_url_status = false;

    if (SYS_MUSIC_START == systemmod.sys_type) {
        CURL* pCURL = curl_easy_init();
        if ( pCURL == NULL ) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Cannot use the curl functions!]\n", __func__);
            } else {
                printf("%s:%d:%s: ERROR [Cannot use the curl functions!]\n", __FILE__, __LINE__, __func__);
            }
            return CURLE_FAILED_INIT;
        }

        curl_easy_setopt(pCURL, CURLOPT_URL, strUrl);
        curl_easy_setopt(pCURL, CURLOPT_HEADER, 0L);
        curl_easy_setopt(pCURL, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
        curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(pCURL, CURLOPT_FOLLOWLOCATION, 1L);
        // curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, songdata_fp_res);
        curl_easy_setopt(pCURL, CURLOPT_SSL_VERIFYPEER, false);    // Do not verify the CA certificate
        curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_LIMIT, 1L);    // Minimum speed limit, if less than 1 byte per second
        curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_TIME, 15L);    // The minimum time limit, if the transmission speed is lower than this speed, it will be suspended after 15s
        curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, 15L);    // Receive data timeout setting, if the data is not received within 15 seconds, exit directly
        curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 15L);    // Link timeout, 15 seconds

        playback_info_stru.url_recv_stru.recv_len = 0;
        memset(playback_info_stru.url_recv_stru.recv_buf, 0, sizeof(playback_info_stru.url_recv_stru.recv_buf));
        curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, write_loading_data);

        if (SYS_MUSIC_START == systemmod.sys_type) {
            ret_curl = curl_easy_perform(pCURL);
            if (CURLE_OK != ret_curl) {
                curl_easy_cleanup(pCURL);

                if (playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [curl_easy_perform is fail! error:%s]\n", __func__, curl_easy_strerror(ret_curl));
                } else {
                    printf("%s:%d:%s: ERROR [curl_easy_perform is fail! error:%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl));
                }
                return ret_curl;
            }
        }

        if (SYS_MUSIC_START == systemmod.sys_type) {
            ret_curl = curl_easy_getinfo(pCURL, CURLINFO_RESPONSE_CODE, &ret_code);
            if (CURLE_OK != ret_curl) {
                if (playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [curl_easy_getinfo is fail! error:%s]\n", __func__, curl_easy_strerror(ret_curl));
                } else {
                    printf("%s:%d:%s: ERROR [curl_easy_getinfo is fail! error:%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl));
                }
            }

            if (200 != ret_code) {
                char* url = NULL;
                curl_easy_getinfo(pCURL, CURLINFO_REDIRECT_URL, &url);
                if (url) {
                    get_redirect_url_status = true;

                    if (playback_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "Redirect url = %d", url);
                    } else {
                        INFO("Redirect url = %d", url);
                    }

                    string str_url(url);
                    memcpy(Redirect_url, str_url.c_str(), str_url.length());

                    if (playback_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "get redirect url success!");
                    } else {
                        DEBUG("get redirect url success!");
                    }
                }
            }
        }
        curl_easy_cleanup(pCURL);
    }

    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s:get song data success!, song size:[%d]", __func__, playback_info_stru.url_recv_stru.recv_len);
    } else {
        DEBUG("%s:get song data success!, song size:[%d]", __func__, playback_info_stru.url_recv_stru.recv_len);
    }

    return ret_curl;
}

int Play_Audio_Data::play_music(char *song_buf, int song_buf_len) {
    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: begin play music mp3 ===>>> pcm !", __func__);
    } else {
        DEBUG("%s: begin play music mp3 ===>>> pcm !", __func__);
    }

    int ret = 0;
    if (SYS_MUSIC_START == systemmod.sys_type) {
        playback_info_stru.music_stru.swr_ctx = swr_alloc();    // Re-sampling
        if (!playback_info_stru.music_stru.swr_ctx) {
            ret = AVERROR(ENOMEM);

            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Could not allocate resample context! ret:%d]\n", __func__, ret);
            } else {
                printf("%s:%d:%s: ERROR [Could not allocate resample context! ret:%d]\n", __FILE__, __LINE__, __func__, ret);
            }
            goto end;
        }

        /* set options */
        av_opt_set_int(playback_info_stru.music_stru.swr_ctx, "in_channel_layout", playback_info_stru.music_stru.src_ch_layout, 0);
        av_opt_set_int(playback_info_stru.music_stru.swr_ctx, "in_sample_rate", playback_info_stru.music_stru.src_rate , 0);
        av_opt_set_sample_fmt(playback_info_stru.music_stru.swr_ctx, "in_sample_fmt", playback_info_stru.music_stru.src_sample_fmt, 0);

        av_opt_set_int(playback_info_stru.music_stru.swr_ctx, "out_channel_layout", playback_info_stru.music_stru.dst_ch_layout, 0);
        av_opt_set_int(playback_info_stru.music_stru.swr_ctx, "out_sample_rate", playback_info_stru.music_stru.dst_rate, 0);
        av_opt_set_sample_fmt(playback_info_stru.music_stru.swr_ctx, "out_sample_fmt", playback_info_stru.music_stru.dst_sample_fmt, 0);

        /* Initialize the resampling context */
        if ((ret = swr_init(playback_info_stru.music_stru.swr_ctx)) < 0) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Failed to initialize the resampling context!ret:%d]\n", __func__, ret);
            } else {
                printf("%s:%d:%s: ERROR [Failed to initialize the resampling context!ret:%d]\n", __FILE__, __LINE__, __func__, ret);
            }
            goto end;
        }

        /* Allocate source and target sample buffers */
        playback_info_stru.music_stru.src_nb_channels = av_get_channel_layout_nb_channels(playback_info_stru.music_stru.src_ch_layout);
        ret = av_samples_alloc_array_and_samples(&playback_info_stru.music_stru.src_data,
                                                &playback_info_stru.music_stru.src_linesize,
                                                playback_info_stru.music_stru.src_nb_channels,
                                                playback_info_stru.music_stru.src_nb_samples,
                                                playback_info_stru.music_stru.src_sample_fmt, 0);
        if (ret < 0) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Could not allocate source samples!ret:%d]\n", __func__, ret);
            } else {
                printf("%s:%d:%s: ERROR [Could not allocate source samples!ret:%d]\n",  __FILE__, __LINE__, __func__, ret);
            }
            goto end;
        }

        /* Calculate the number of converted samples: avoid buffering and ensure that the output
        * buffer contains at least all the converted input samples 
        */
        playback_info_stru.music_stru.max_dst_nb_samples = playback_info_stru.music_stru.dst_nb_samples = av_rescale_rnd(playback_info_stru.music_stru.src_nb_samples,
                                                                                                                        playback_info_stru.music_stru.dst_rate,
                                                                                                                        playback_info_stru.music_stru.src_rate,
                                                                                                                        AV_ROUND_UP);

        /* The buffer will be written directly to the rawaudio file, without alignment */
        playback_info_stru.music_stru.dst_nb_channels = av_get_channel_layout_nb_channels(playback_info_stru.music_stru.dst_ch_layout);
        ret = av_samples_alloc_array_and_samples(&playback_info_stru.music_stru.dst_data,
                                                &playback_info_stru.music_stru.dst_linesize,
                                                playback_info_stru.music_stru.dst_nb_channels,
                                                playback_info_stru.music_stru.dst_nb_samples,
                                                playback_info_stru.music_stru.dst_sample_fmt, 0);
        if (ret < 0) {
            if (playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: ERROR [Could not allocate destination samples!ret:%d]\n", __func__, ret);
            } else {
                printf("%s:%d:%s: ERROR [Could not allocate destination samples!ret:%d]\n",  __FILE__, __LINE__, __func__, ret);
            }
            goto end;
        }

        if (SYS_MUSIC_START == systemmod.sys_type) {
            pthread_mutex_lock(&playback_info_stru.mutex_and_cond_stru.voice_mutex);
            ret = decode((unsigned char *)song_buf, song_buf_len);
            pthread_mutex_unlock(&playback_info_stru.mutex_and_cond_stru.voice_mutex);
        }

    end:
        if (playback_info_stru.music_stru.src_data) {
            av_freep(&playback_info_stru.music_stru.src_data[0]);
        }

        av_freep(&playback_info_stru.music_stru.src_data);
        playback_info_stru.music_stru.src_data = NULL;

        if (playback_info_stru.music_stru.dst_data) {
            av_freep(&playback_info_stru.music_stru.dst_data[0]);
        }

        av_freep(&playback_info_stru.music_stru.dst_data);
        playback_info_stru.music_stru.dst_data = NULL;

        swr_free(&playback_info_stru.music_stru.swr_ctx);
        playback_info_stru.music_stru.swr_ctx = NULL;
    }

    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(playback_info_stru.sendlogobj, DEBUG, "%s: decode mp3 play music end!", __func__);
    } else {
        DEBUG("%s: decode mp3 play music end!", __func__);
    }
    return ret;
}

int Play_Audio_Data::decode(unsigned char const *start, unsigned long length) {
    struct mad_decoder decoder;
    int result;

    playback_info_stru.music_stru.mad_fun_buf.start = start;
    playback_info_stru.music_stru.mad_fun_buf.length = length;

    snd_pcm_prepare(playback_info_stru.snd_pcm_stru._play_handle);

    mad_decoder_init(&decoder, &playback_info_stru.music_stru.mad_fun_buf, input, 0 , 0, output, error, 0);

    result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

    mad_decoder_finish(&decoder);    //  release the decoder

    snd_pcm_drop(playback_info_stru.snd_pcm_stru._play_handle);

    if (playback_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:mp3 转 pcm 播放解码函数调用结束！", __func__);
    } else {
        DEBUG("%s:mp3 转 pcm 播放解码函数调用结束！", __func__);
    }
    return result;
}

enum mad_flow Play_Audio_Data::error(void *data, struct mad_stream *stream, struct mad_frame *frame) {
    if (SYS_MUSIC_START != systemmod.sys_type) {
        return MAD_FLOW_BREAK;
    }

    return MAD_FLOW_CONTINUE;
}

// This is the input callback. The purpose of this callback function is to (re)fill the stream buffer to be decoded
enum mad_flow Play_Audio_Data::input(void *data, struct mad_stream *stream) {
    MAD_FUNC_BUFFER *buffer = reinterpret_cast<MAD_FUNC_BUFFER *>(data);
    if (!buffer->length) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:播放mp3 转 pcm 解码时，读入数据长度为0 停止播放！" , __func__);
        } else {
            INFO("%s:播放mp3 转 pcm 解码时，读入数据长度为0 停止播放！" , __func__);
        }
        return MAD_FLOW_STOP;
    }

    if (SYS_MUSIC_START != systemmod.sys_type) {
        buffer->length = 0;
        return MAD_FLOW_BREAK;
    }

    mad_stream_buffer(stream, buffer->start, buffer->length);

    if (SYS_MUSIC_START != systemmod.sys_type) {
        buffer->length = 0;
        return MAD_FLOW_BREAK;
    }

    buffer->length = 0;
    return MAD_FLOW_CONTINUE;
}

//  Reduce the high-resolution samples of MAD to 16 bits
inline signed int Play_Audio_Data::scale(mad_fixed_t sample) {
    //  round
    sample += (1L << (MAD_F_FRACBITS - 16));

    //  clip
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    //  quantize
    return sample >> (MAD_F_FRACBITS + 1 - 16);
}

enum mad_flow Play_Audio_Data::output(void *data, struct mad_header const *header, struct mad_pcm *pcm) {
    if (SYS_MUSIC_STOP == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
        } else {
            INFO("%s:系统切换至停止播放音乐状态！", __func__);
        }

        return MAD_FLOW_BREAK;
    } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_MUSIC_START != systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
        } else {
            INFO("%s:系统切换至停止播放音乐状态！", __func__);
        }
        return MAD_FLOW_BREAK;
    }

    int ret = 0;
    unsigned int nchannels, nsamples, n;
    mad_fixed_t const *left_ch, *right_ch;

    // pcm->samplerate contains the sampling frequency
    nchannels = pcm->channels;
    n = nsamples = pcm->length;
    left_ch = pcm->samples[0];
    right_ch = pcm->samples[1];

    unsigned char Output[6912], *OutputPtr;
    OutputPtr = Output;

    while (nsamples--) {
        signed int sample;

        // Add stop
        if (SYS_MUSIC_STOP == systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
            } else {
                INFO("%s:系统切换至停止播放音乐状态！", __func__);
            }
            return MAD_FLOW_BREAK;
        } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
            } else {
                INFO("%s:语音唤醒音乐暂停播放！", __func__);
            }
            return MAD_FLOW_BREAK;
        } else if (SYS_MUSIC_START != systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
            } else {
                INFO("%s:语音唤醒音乐暂停播放！", __func__);
            }
            return MAD_FLOW_BREAK;
        }

        //  output sample(s) in 16-bit signed little-endian PCM
        sample = scale(*left_ch++);

        *(OutputPtr++) = sample >> 0;
        *(OutputPtr++) = sample >> 8;
        if (nchannels == 2) {
            sample = scale(*right_ch++);
            *(OutputPtr++) = sample >> 0;
            *(OutputPtr++) = sample >> 8;
        }

        // Add stop
        if (SYS_MUSIC_STOP == systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
            } else {
                INFO("%s:系统切换至停止播放音乐状态！", __func__);
            }
            return MAD_FLOW_BREAK;
        } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
            } else {
                INFO("%s:语音唤醒音乐暂停播放！", __func__);
            }
            return MAD_FLOW_BREAK;
        } else if (SYS_MUSIC_START != systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
            } else {
                INFO("%s:语音唤醒音乐暂停播放！", __func__);
            }
            return MAD_FLOW_BREAK;
        }
    }

    // Add stop
    if (SYS_MUSIC_STOP == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
        } else {
            INFO("%s:系统切换至停止播放音乐状态！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_MUSIC_START != systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    }

    OutputPtr = Output;

    // Add stop
    if (SYS_MUSIC_STOP == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
        } else {
            INFO("%s:系统切换至停止播放音乐状态！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_MUSIC_START != systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    }

    //  Generate synthetic audio
    memcpy(reinterpret_cast<double*>(static_obj->playback_info_stru.music_stru.src_data[0]), reinterpret_cast<uint8_t *>(OutputPtr), n * 4);
    if (SYS_MUSIC_STOP == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
        } else {
            INFO("%s:系统切换至停止播放音乐状态！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_MUSIC_START != systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    }

    //  Calculate the target number of samples
    static_obj->playback_info_stru.music_stru.dst_nb_samples = av_rescale_rnd(swr_get_delay(static_obj->playback_info_stru.music_stru.swr_ctx,
                                                                                            static_obj->playback_info_stru.music_stru.src_rate) + static_obj->playback_info_stru.music_stru.src_nb_samples,
                                                                                            static_obj->playback_info_stru.music_stru.dst_rate,
                                                                                            static_obj->playback_info_stru.music_stru.src_rate,
                                                                                            AV_ROUND_UP);
    //  LOG("dst_nb_samples:%d max_dst_nb_samples:%d\n", music_obj->music_stru.dst_nb_samples, music_obj->music_stru. max_dst_nb_samples);

    if (static_obj->playback_info_stru.music_stru.dst_nb_samples > static_obj->playback_info_stru.music_stru.max_dst_nb_samples) {
        if (SYS_MUSIC_STOP == systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
            } else {
                INFO("%s:系统切换至停止播放音乐状态！", __func__);
            }
            return MAD_FLOW_BREAK;
        } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
            } else {
                INFO("%s:语音唤醒音乐暂停播放！", __func__);
            }
            return MAD_FLOW_BREAK;
        } else if (SYS_MUSIC_START != systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
            } else {
                INFO("%s:语音唤醒音乐暂停播放！", __func__);
            }
            return MAD_FLOW_BREAK;
        }

        av_freep(&static_obj->playback_info_stru.music_stru.dst_data[0]);
        ret = av_samples_alloc(static_obj->playback_info_stru.music_stru.dst_data,
                            &static_obj->playback_info_stru.music_stru.dst_linesize,
                            static_obj->playback_info_stru.music_stru.dst_nb_channels,
                            static_obj->playback_info_stru.music_stru.dst_nb_samples,
                            static_obj->playback_info_stru.music_stru.dst_sample_fmt, 1);
        if (ret < 0) {
            if (SYS_MUSIC_STOP == systemmod.sys_type) {
                if (static_obj->playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
                } else {
                    INFO("%s:系统切换至停止播放音乐状态！", __func__);
                }
                return MAD_FLOW_BREAK;
            } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
                if (static_obj->playback_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
                } else {
                    INFO("%s:语音唤醒音乐暂停播放！", __func__);
                }
                return MAD_FLOW_BREAK;
            }

            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s: av_samples_alloc error!\n", __func__);
            } else {
                printf("%s:%d:%s: av_samples_alloc error!\n", __FILE__, __LINE__, __func__);
            }
            return MAD_FLOW_IGNORE;
        }

        static_obj->playback_info_stru.music_stru.max_dst_nb_samples = static_obj->playback_info_stru.music_stru.dst_nb_samples;
    }

    //  Convert to target format
    //  LOG("src_nb_samples:%d dst_nb_samples:%d \n", music_obj->music_stru.src_nb_samples, music_obj->music_stru.dst_nb_samples);
    if (SYS_MUSIC_STOP == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
        } else {
            INFO("%s:系统切换至停止播放音乐状态！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_MUSIC_START != systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    }

    ret = swr_convert(static_obj->playback_info_stru.music_stru.swr_ctx,
                    static_obj->playback_info_stru.music_stru.dst_data,
                    static_obj->playback_info_stru.music_stru.dst_nb_samples,
                    (const uint8_t **)static_obj->playback_info_stru.music_stru.src_data,
                    static_obj->playback_info_stru.music_stru.src_nb_samples);
    if (ret < 0) {
        if (SYS_MUSIC_STOP == systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
            } else {
                INFO("%s:系统切换至停止播放音乐状态！", __func__);
            }

            return MAD_FLOW_BREAK;
        } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
            } else {
                INFO("%s:语音唤醒音乐暂停播放！", __func__);
            }
            return MAD_FLOW_BREAK;
        } else if (SYS_MUSIC_START != systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
            } else {
                INFO("%s:语音唤醒音乐暂停播放！", __func__);
            }
            return MAD_FLOW_BREAK;
        }

        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s: Error while converting\n", __func__);
        } else {
            printf("%s:%d:%s: Error while converting\n", __FILE__, __LINE__, __func__);
        }
        return MAD_FLOW_IGNORE;
    }

    if (SYS_MUSIC_STOP == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
        } else {
            INFO("%s:系统切换至停止播放音乐状态！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_MUSIC_START != systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    }

    static_obj->playback_info_stru.music_stru.dst_bufsize = av_samples_get_buffer_size(&static_obj->playback_info_stru.music_stru.dst_linesize,
                                                                                    static_obj->playback_info_stru.music_stru.dst_nb_channels,
                                                                                    ret, static_obj->playback_info_stru.music_stru.dst_sample_fmt, 1);
    if (static_obj->playback_info_stru.music_stru.dst_bufsize < 0) {
        if (SYS_MUSIC_STOP == systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
            } else {
                INFO("%s:系统切换至停止播放音乐状态！", __func__);
            }

            return MAD_FLOW_BREAK;
        } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
            } else {
                INFO("%s:语音唤醒音乐暂停播放！", __func__);
            }
            return MAD_FLOW_BREAK;
        } else if (SYS_MUSIC_START != systemmod.sys_type) {
            if (static_obj->playback_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
            } else {
                INFO("%s:语音唤醒音乐暂停播放！", __func__);
            }
            return MAD_FLOW_BREAK;
        }

        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s: Could not get sample buffer size\n", __func__);
        } else {
            printf("%s:%d:%s: Could not get sample buffer size\n", __FILE__, __LINE__, __func__);
        }
        return MAD_FLOW_IGNORE;
    }

    if (SYS_MUSIC_STOP == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
        } else {
            INFO("%s:系统切换至停止播放音乐状态！", __func__);
        }

        return MAD_FLOW_BREAK;
    } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_MUSIC_START != systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    }

    char pcm_data[static_obj->playback_info_stru.music_stru.dst_bufsize + 1];
    memset(pcm_data, 0, sizeof(pcm_data));
    memcpy(pcm_data, static_obj->playback_info_stru.music_stru.dst_data[0], static_obj->playback_info_stru.music_stru.dst_bufsize);

    if (SYS_MUSIC_STOP == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
        } else {
            INFO("%s:系统切换至停止播放音乐状态！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_MUSIC_START != systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    }

    ret = snd_pcm_writei(static_obj->playback_info_stru.snd_pcm_stru._play_handle, pcm_data, static_obj->playback_info_stru.music_stru.dst_bufsize/2);
    if (ret == -EPIPE) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s: underrun occurred\n", __func__);
        } else {
            printf("%s:%d:%s: underrun occurred\n", __FILE__, __LINE__, __func__);
        }
        snd_pcm_prepare(static_obj->playback_info_stru.snd_pcm_stru._play_handle);
    } else if (ret < 0) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s: error from ffmpeg writei: %s\n", __func__, snd_strerror(ret));
        } else {
            printf("%s:%d:%s: error from ffmpeg writei: %s\n", __FILE__, __LINE__, __func__, snd_strerror(ret));
        }
        //  snd_pcm_prepare(static_obj->playback_info_stru.snd_pcm_stru._play_handle);
    } else if (ret !=  static_obj->playback_info_stru.music_stru.dst_bufsize/2) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s: ffmpeg short writei, writei %d _playback_frames", __func__, ret);
        } else {
            printf("%s:%d:%s: ffmpeg short writei, writei %d _playback_frames", __FILE__, __LINE__, __func__, ret);
        }
    }

    // Add stop
    if (SYS_MUSIC_STOP == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
        } else {
            INFO("%s:系统切换至停止播放音乐状态！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_MUSIC_START != systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    }

    OutputPtr = Output;

    if (SYS_MUSIC_STOP == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:系统切换至停止播放音乐状态！", __func__);
        } else {
            INFO("%s:系统切换至停止播放音乐状态！", __func__);
        }
        return MAD_FLOW_BREAK;
    } else if (SYS_VOICE_RECOGNIZE == systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }

        return MAD_FLOW_BREAK;
    } else if (SYS_MUSIC_START != systemmod.sys_type) {
        if (static_obj->playback_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(static_obj->playback_info_stru.sendlogobj, DEBUG, "%s:语音唤醒音乐暂停播放！", __func__);
        } else {
            INFO("%s:语音唤醒音乐暂停播放！", __func__);
        }
        return MAD_FLOW_BREAK;
    }

    return MAD_FLOW_CONTINUE;
}

std::string Play_Audio_Data::urlencode(std::string &str_source) {
    char const *in_str = str_source.c_str();
    int in_str_len = strlen(in_str);
    int out_str_len = 0;
    std::string out_str;
    register unsigned char c;
    unsigned char *to, *start;
    unsigned char const *from, *end;
    unsigned char hexchars[] = "0123456789ABCDEF";

    from = (unsigned char *)in_str;
    end = (unsigned char *)in_str + in_str_len;
    start = to = (unsigned char *) malloc(3*in_str_len+1);

    while (from < end) {
        c = *from++;

        if (c == ' ') {
            *to++ = '+';
        } else if ((c < '0' && c != '-' && c != '.' && c != '/' && c != '&') || (c < 'A' && c > '9' && c != ':' && c!= '?' && c != '=') || (c > 'Z' && c < 'a' && c != '_') || (c > 'z')) {
            to[0] = '%';
            to[1] = hexchars[c >> 4];
            to[2] = hexchars[c & 15];
            to += 3;
        } else {
            *to++ = c;
        }
    }
    *to = 0;

    out_str_len = to - start;
    out_str = reinterpret_cast<char*>(start);
    free(start);
    start = to = NULL;

    return out_str;
}

