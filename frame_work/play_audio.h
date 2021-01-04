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
#ifndef FRAME_WORK_PLAY_AUDIO_H_
#define FRAME_WORK_PLAY_AUDIO_H_

#include <alsa/asoundlib.h>
#include <iostream>
#include <string>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "mad.h"
#include "opt.h"
#include "channel_layout.h"
#include "samplefmt.h"
#include "swresample.h"

#ifdef __cplusplus
}
#endif

#include "frame_work_common_def.h"

#include "client_log_dump.h"

#define LOCAL_VOICE_FILE_NUM 11  //  Number of local audio files
#define MAX_URL_SONG_MSG_LEN 40 *1024 *1024   //  40M Size to store music data

#define TOKEN_URL "XXX"  //  "https://xxx.xxx.com.cn/musicSearch/appLogin?userId=xxxx&deviceId=xxxx&deviceType=xxx"

#define MUSIC_LIST_URL_ARG "XXX"  //  "https://xxx.xxx.com.cn/musicSearch/audio/querySongs?songName=%s&artistName=%s&domainIdNLU=xxx"

#define NO_MUSIC_LIST 100     //  No music list
#define MUSIC_LIST_END 99     //  Music list finished

#define GET_MUSIC_FAIL 98     //  Failed to get music
#define GET_SONG_DATA_ERR 97  //  Failed to get music data

#define LOCAL_MUSIC_PATH "../local_music"

typedef struct playback_mutex_and_cond {
    pthread_mutex_t playback_mutex;    //  = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t playback_cond;      //  = PTHREAD_COND_INITIALIZER;

    pthread_mutex_t music_mutex;
    pthread_cond_t music_cond;

    pthread_mutex_t voice_mutex;       //  Play sound lock to ensure that there is only one task to play sound at a time
}PLAYBACK_MUTEX_AND_COND;

typedef struct snd_pcm_info_stru {
    snd_pcm_t *_play_handle = NULL;  //  PCI device handle
    snd_pcm_hw_params_t *_playback_params = NULL; //  Hardware information and PCM stream configuration
    snd_pcm_uframes_t _playback_frames = 2560;

    int _datablock = 2;
    int _playback_dir = 0;
    int _playback_buf_size = 0;

    unsigned int _play_channels = 0;
    unsigned int _frequency = 16000;

    char * _play_buffer = NULL;
}SND_PCM_INFO_STRU;

//  libmad settings related functions
typedef struct mad_func_buffer {
unsigned char const *start;
unsigned long length;
}MAD_FUNC_BUFFER;

//  Play network songs mp3 convert pcm play related information
typedef struct music_info_stru {
    struct SwrContext * swr_ctx;
    int64_t src_ch_layout = AV_CH_LAYOUT_STEREO;
    int64_t dst_ch_layout = AV_CH_LAYOUT_MONO;  //  AV_CH_LAYOUT_STEREO;
    int src_rate = 44100;
    int dst_rate = 16000;
    enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16;
    enum AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16;
    int src_nb_channels = 0;
    int dst_nb_channels = 0;
    uint8_t **src_data = NULL;
    uint8_t **dst_data = NULL;
    int src_linesize = 0;
    int dst_linesize = 0;
    int src_nb_samples = 1152;
    int dst_nb_samples;
    int max_dst_nb_samples;
    int dst_bufsize = 0;
    int music_ret = 0;
    MAD_FUNC_BUFFER mad_fun_buf;
}MUSIC_INFO_STRU;

//  The data storage structure of the server response after the URL request service
typedef struct url_recv_info {
    char recv_buf[MAX_URL_SONG_MSG_LEN +1] = {0};  //  The server returns the information storage buf
    int recv_len = 0;                              //  Length of information returned by the server
}URL_RECV_INFO;

enum LOCAL_VOICE_FILE {
    SYS_VOICE,
    AWAKE_VOICE,
    NO_HEAR_VOICE,
    NOT_UNDERSTAND_VOICE,
    NET_ERR_VOICE,
    MUSIC_LIST_NULL,
    MUSIC_END_LIST_NULL,
    GET_MUSIC_FAIL_VOICE,
    GET_SONG_DADA_ERR_VOICE,
    CLOCK_VOICE,
    CLOCK_SET_SUCESS_VOICE
};

typedef struct playback_info {
    pthread_t playback_thread_id;                 //  playback thread id
    pthread_t music_thread_id;                    //  play music thread id
    const int song_name_len = 100;
    const int artist_len = 50;
    char song_name[100] = {0};
    char artist[50] = {0};
    char tts_voice_buf[TTS_VOICE_LEN + 160 * 2 * 1 * 50] = {0};
    int tts_voice_len = 0;
    char clock_tts_voice_buf[TTS_VOICE_LEN + 160 * 2 * 1 * 50] = {0};
    int clock_tts_voice_len = 0;
    volatile bool client_send_log_status = false;
    volatile bool pause_status = false;                      //  true:Stop the current task
    volatile bool play_thread_action = false;
    volatile bool music_net_err = false;                     //  true:There is a problem with the network playing online music
    int music_proc_ret_status = 0;                           //  There is a problem with the network playing online music
    volatile PLAY_ACTION  action_type = PLAY_DEFAULT;
    volatile int play_music_count = 0;   //  Request music count
    FILE * pcm_file[LOCAL_VOICE_FILE_NUM] = {nullptr,  nullptr,  nullptr,  nullptr,  nullptr,  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};  //  Local audio file pointer
    PLAYBACK_MUTEX_AND_COND mutex_and_cond_stru;  //  Mutex, condition variable
    SND_PCM_INFO_STRU snd_pcm_stru;               //  Play pcm data information structure
    URL_RECV_INFO url_recv_stru;
    MUSIC_INFO_STRU music_stru;
    SendLogProc * sendlogobj = NULL;
}PLAYBACK_INFO;

class Play_Audio_Data {
 public:
    Play_Audio_Data();
    ~Play_Audio_Data();

    int Init();
    int DeInit();

    int pause();
    int SendSig(PLAY_ACTION play_cation);
    int set_tts_voice_info(char *data,  int len);
    int set_clock_tts_voice_info(char *data,  int len);
    int set_music_info(char *song,  int song_len,  char *artist,  int artist_len);

    CURLcode App_Binding_Token(const std::string & strUrl,  int nTimeout,  const char *token_nm);  //    绑定酷狗账户

 private:
    int thread_init();
    void thread_deinit();
    int play_snd_pcm_init();
    void play_snd_pcm_deinit();

    void mutex_and_cond_init();
    void mutex_and_cond_deinit();

    int local_pcm_init();
    void local_pcm_deinit();

    int get_local_music();
    void getFiles(std::string &path, std::vector<std::string>& files);
    void music_info_voice();
    int check_local_music(std::string &s);
    int get_local_music_data(std::string &music_file);
    int play_local_music();

    int net_music_proc();

    CURLcode Loading_Cms_Token(const char * strUrl,  int timeout, long &ret_code);
    int analysis_token_id(char * token_id);
    CURLcode Get_MusicList(const std::string & strUrl,  int nTimeout,  char *token_nm,  long &ret_code);
    int analysis_music_info(char * token_id);
    int music_proc(cJSON * &rows_list,  char * &token_id);
    CURLcode Get_SongUrl(const std::string &strUrl,  int nTimeout,  char *token_nm,  long &ret_code);
    CURLcode Get_SongData(char * strUrl,  int nTimeout,  long &ret_code,  char * Redirect_url,  int Redirect_url_len,  bool &get_redirect_url_status);
    std::string urlencode(std::string &str_source);

    int play_music(char *song_buf,  int song_buf_len);
    int decode(unsigned char const *start,  unsigned long length);

    static enum mad_flow error(void *data,  struct mad_stream *stream,  struct mad_frame *frame);
    static enum mad_flow input(void *data,  struct mad_stream *stream);
    static enum mad_flow output(void *data,  struct mad_header const *header,  struct mad_pcm *pcm);
    static inline signed int scale(mad_fixed_t sample);

    static int write_loading_data(void* buffer,  int size,  int nmemb,  void **stream);

    static void * playback_thread_proc(void *arg);
    static void * music_thread_proc(void *arg);

    int play_awake_pcm(FILE * voice_file);
    int play_pcm(FILE * voice_file);
    int play_pcm(const char * voice_buf,  int len);

    static Play_Audio_Data * static_obj;
    PLAYBACK_INFO playback_info_stru;

    std::vector<std::string> local_music_files;
};

#endif  //  FRAME_WORK_PLAY_AUDIO_H_
