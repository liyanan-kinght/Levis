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

#ifndef FRAME_WORK_TEST_ENV_H_
#define FRAME_WORK_TEST_ENV_H_

#include "frame_work_common_def.h"

#define TOKEN_URL "XXX"  //  "https://xxx.xxx.com.cn/musicSearch/appLogin?userId=xxxx&deviceId=xxxx&deviceType=xxxx"
#define MUSIC_LIST_URL_ARG "XXX"  //  "https://xxx.xxx.com.cn/musicSearch/audio/querySongs?songName=%s&artistName=%s&domainIdNLU=xxxx"
#define MAX_URL_SONG_MSG_LEN 40 *1024 *1024   //  40m 大小来存储音乐数据

#define ENV_TTS_ERR_VOICE "env_tts_err_voice.pcm"
#define ENV_ASR_ERR_VOICE "env_asr_err_voice.pcm"
#define ENV_MUSIC_ERR_VOICE "env_music_err_voice.pcm"
#define ENV_ALL_OK_VOICE "env_all_ok_voice.pcm"

typedef struct test_curl_val {
    CURL *_pCurl = NULL;  //  curl handle
    struct curl_slist * _header = NULL;
    struct curl_slist * _list_par = NULL; 
    struct curl_slist * _list_voi = NULL;
    struct curl_httppost * _formpost = NULL;
    struct curl_httppost * _last_ptr = NULL;
}TEST_CURL_VAL;

//  The structure of the data received by the curl server
typedef struct test_recv_buf {
    char *memory = NULL;  //  Receive data storage address
    size_t size = 0;  //  Receive data size
}TEST_CALLBACK_RECV_BUF;


typedef struct snd_pcm_info_test {
    snd_pcm_t *_play_handle = NULL;  //  PCI device handle  
    snd_pcm_hw_params_t *_playback_params = NULL;  //  Hardware information and PCM stream configuration
    snd_pcm_uframes_t _playback_frames = 2560;

    int _datablock = 2;
    int _playback_dir = 0;
    int _playback_buf_size = 0;

    unsigned int _play_channels = 0;
    unsigned int _frequency = 16000;

    char * _play_buffer = NULL;
}SND_PCM_INFO_TEST;

enum class music_ret:int {
    //  File pointer array footer
    ENV_TTS_ERR,
    ENV_ASR_ERR,
    ENV_MUSIC_ERR,
    ENV_ALL_OK,

    //  asr Recognition result
    ENV_ASR_DISCERN_VOICE,
    ENV_ASR_DISCERN_VOICE_ERR,
    ENV_ASR_DISCERN_VOICE_OTHER,  //  Recognized speech text translation non-target speech text description
    ENV_ASR_RES_NULL,

    //  Music acquisition situation
    NO_MUSIC_LIST_T = 97,       //  No music list
    MUSIC_LIST_END_T = 98,      //  Music list finished

    GET_MUSIC_FAIL_T = 99,      //  Failed to get music data
    GET_SONG_DATA_ERR_T = 100  //  Failed to get music data
};

class TestEnvProc{
public:
    int Init();
    int DeInit();
    int ProcTestEnv();

private:
    int local_pcm_file_init();
    int local_pcm_file_deinit();

    //  test  asr
    int asr_curl_init();
    int asr_curl_deinit();
    int asr_curl_session_init();
    int asr_curl_session_deinit();
    int asr_curl_session_proc();
    static int asr_write_data(void *buffer,  int size,  int nmemb, void*stream);
    int analysis_asr_result(char *in_buf);

    //  test tts
    int tts_curl_init();
    int tts_curl_deinit();
    int tts_curl_session_init();
    int tts_curl_session_deinit();
    int tts_curl_session_proc();
    static int tts_write_data(void *buffer,  int size,  int nmemb, void*stream);

    //  test asr
    TEST_CURL_VAL asr_handle_curl;
    static TEST_CALLBACK_RECV_BUF asr_recv;

    //   test tts
    TEST_CURL_VAL tts_handle_curl;
    static TEST_CALLBACK_RECV_BUF tts_recv;

    int kugou_music_init();
    int kugou_music_deinit();
    int kugou_music_proc();

    CURLcode Loading_Cms_Token(const char * strUrl,  int timeout, long &ret_code);
    CURLcode Get_MusicList(const std::string & strUrl,  int nTimeout,  char *token_nm, long &ret_code);
    CURLcode Get_SongData( char * strUrl,  int nTimeout, long &ret_code, char * Redirect_url, int Redirect_url_len, bool &get_redirect_url_status);
    CURLcode Get_SongUrl(const std::string &strUrl,  int nTimeout,  char *token_nm, long &ret_code);
    int music_proc(cJSON * &rows_list, char * &token_id);
    int analysis_music_info(char * token_id);
    int analysis_token_id(char * token_id);
    static int write_loading_data(void* buffer,  int size,  int nmemb,  void *stream);

    static TEST_CALLBACK_RECV_BUF music_recv;

    int play_snd_pcm_init();
    int play_snd_pcm_deinit();
    int play_pcm(FILE * voice_file);

    SND_PCM_INFO_TEST snd_handle;
    FILE * file[4] = {0};
};

#endif  // FRAME_WORK_TEST_ENV_H_
