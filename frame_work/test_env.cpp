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

#include <iostream>
#include <string>

#ifdef  __cplusplus
extern "C" {
#endif
#include <string.h>
#include <alsa/asoundlib.h>

#ifdef  __cplusplus
}
#endif

#include "test_env.h"

#define ASRPOSTASRURL  "https://voice.lenovomm.com/lasf/asr"

#define CURL_RECV_TTS_VOICE_LEN  (160 * 2 * 1 * 2000)        //  tts url recv voice buf len   Mono

#define TTS_POSTURL  "https://voice.lenovo.com/lasf/cloudtts"

TEST_CALLBACK_RECV_BUF TestEnvProc::music_recv;
TEST_CALLBACK_RECV_BUF TestEnvProc::tts_recv;
TEST_CALLBACK_RECV_BUF TestEnvProc::asr_recv;

std::string encode_to_url(std::string &str_source) {
    char const *in_str = str_source.c_str();
    int in_str_len = strlen(in_str);
    int out_str_len = 0;
    std::string out_str;
    register unsigned char c;
    unsigned char *to,  *start;
    unsigned char const *from,  *end;
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
    out_str = (char *) start;
    free(start);
    start = to = NULL;

    return out_str;
}

int TestEnvProc::Init() {
    if (-1 == play_snd_pcm_init()) {
        return -1;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    asr_curl_init();
    tts_curl_init();

    if (-1 == local_pcm_file_init()) {
        return -1;
    }

    kugou_music_init();
    return 0;
}

int TestEnvProc::DeInit() {
    play_snd_pcm_deinit();
    asr_curl_deinit();

    tts_curl_deinit();
    local_pcm_file_deinit();
    kugou_music_deinit();

    curl_global_cleanup();
    return 0;
}

int TestEnvProc::local_pcm_file_init() {
    file[static_cast<int>(music_ret::ENV_TTS_ERR)] = fopen(ENV_TTS_ERR_VOICE, "rb");
    if (nullptr == file[static_cast<int>(music_ret::ENV_TTS_ERR)]) {
        std::cout << "read " ENV_TTS_ERR_VOICE "error!" << std::endl;
        return -1;
    }

    file[static_cast<int>(music_ret::ENV_ASR_ERR)] = fopen(ENV_ASR_ERR_VOICE, "rb");
    if (nullptr == file[static_cast<int>(music_ret::ENV_ASR_ERR)]) {
        std::cout << "read " ENV_ASR_ERR_VOICE "error!" << std::endl;
        return -1;
    }

    file[static_cast<int>(music_ret::ENV_MUSIC_ERR)] = fopen(ENV_MUSIC_ERR_VOICE, "rb");
    if (nullptr == file[static_cast<int>(music_ret::ENV_MUSIC_ERR)]) {
        std::cout << "read " ENV_MUSIC_ERR_VOICE "error!" << std::endl;
        return -1;
    }

    file[static_cast<int>(music_ret::ENV_ALL_OK)] = fopen(ENV_ALL_OK_VOICE, "rb");
    if (nullptr == file[static_cast<int>(music_ret::ENV_ALL_OK)]) {
        std::cout << "read " ENV_ALL_OK_VOICE "error!" << std::endl;
        return -1;
    }

    return 0;
}

int TestEnvProc::local_pcm_file_deinit() {
    if (nullptr != file[static_cast<int>(music_ret::ENV_TTS_ERR)])
        fclose(file[static_cast<int>(music_ret::ENV_TTS_ERR)]);

    if (nullptr != file[static_cast<int>(music_ret::ENV_ASR_ERR)])
        fclose(file[static_cast<int>(music_ret::ENV_ASR_ERR)]);

    if (nullptr != file[static_cast<int>(music_ret::ENV_MUSIC_ERR)])
        fclose(file[static_cast<int>(music_ret::ENV_MUSIC_ERR)]);

    if (nullptr != file[static_cast<int>(music_ret::ENV_ALL_OK)])
        fclose(file[static_cast<int>(music_ret::ENV_ALL_OK)]);

    return 0;
}

int TestEnvProc::ProcTestEnv() {
    int err_conut = 0;
    int count_num = 0;
    const int request_num_max = 3;
    int asr_res = 0;

    while (request_num_max > count_num) {
        asr_res = 0;
        asr_curl_session_init();
        asr_res = asr_curl_session_proc();
        asr_curl_session_deinit();

        switch (asr_res) {
            case static_cast<int>(music_ret::ENV_ASR_DISCERN_VOICE):
                std::cout << "asr res success!" << std::endl;
                count_num += request_num_max;
                break;

            case static_cast<int>(music_ret::ENV_ASR_DISCERN_VOICE_ERR):
            case static_cast<int>(music_ret::ENV_ASR_DISCERN_VOICE_OTHER):
            case static_cast<int>(music_ret::ENV_ASR_RES_NULL):
                std::cout << "asr res err!" << std::endl;
                ++count_num;
                if (request_num_max <= count_num) {
                    play_pcm(file[static_cast<int>(music_ret::ENV_ASR_ERR)]);
                    ++err_conut;
                }
                break;

            default:
                std::cout << "default!" << std::endl;
                count_num += request_num_max;
                play_pcm(file[static_cast<int>(music_ret::ENV_ASR_ERR)]);
                ++err_conut;
                break;
        }
    }

    count_num = 0;
    while (1) {
        if (-1 == tts_curl_session_proc()) {
            std::cout << "tts communication err!" << std::endl;
            ++count_num;
            if (request_num_max == count_num) {
                play_pcm(file[static_cast<int>(music_ret::ENV_TTS_ERR)]);
                ++err_conut;
                break;
            }
        } else {
            break;
        }
    }

    if (!PLAY_LOCAL_MUSIC) {
        count_num = 0;
        while (1) {
            if (-1 == kugou_music_proc()) {
                std::cout << "kugou communication err!" << std::endl;
                ++count_num;
                if (request_num_max == count_num) {
                    play_pcm(file[static_cast<int>(music_ret::ENV_MUSIC_ERR)]);
                    ++err_conut;
                    break;
                }
            } else {
                break;
            }
        }
    }

    if (!err_conut) {
        play_pcm(file[static_cast<int>(music_ret::ENV_ALL_OK)]);
    }

    return 0;
}

int TestEnvProc::play_snd_pcm_init() {
    int rec = snd_pcm_open(&snd_handle._play_handle,  "plughw:1, 0",  SND_PCM_STREAM_PLAYBACK,  0);
    if (rec < 0) {
        printf("%s:%d:%s: ERROR [unable to open pcm device: %s]\n",  __FILE__,  __LINE__,  __func__,   snd_strerror(rec));
        return -1;
    }

    snd_pcm_hw_params_alloca(&snd_handle._playback_params);
    snd_pcm_hw_params_any(snd_handle._play_handle,  snd_handle._playback_params);
    rec = snd_pcm_hw_params_set_access(snd_handle._play_handle,  snd_handle._playback_params,  SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rec < 0) {
        printf("%s:%d:%s: ERROR [unable to open pcm device: %s]\n",  __FILE__,  __LINE__,  __func__,  snd_strerror(rec));
        return -1;
    }

    snd_pcm_hw_params_set_format(snd_handle._play_handle,  snd_handle._playback_params,  SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(snd_handle._play_handle,  snd_handle._playback_params,  1);

    int rc = snd_pcm_hw_params_set_rate_near(snd_handle._play_handle,  snd_handle._playback_params,  &snd_handle._frequency,  &snd_handle._playback_dir);
    if (rc < 0) {
        printf("%s:%d:%s: ERROR [set rate failed: %s]\n",  __FILE__,  __LINE__,  __func__,  snd_strerror(rc));
        return -1;
    }

    snd_pcm_hw_params_set_period_size_near(snd_handle._play_handle,  snd_handle._playback_params,  &snd_handle._playback_frames,  &snd_handle._playback_dir);

    rc = snd_pcm_hw_params(snd_handle._play_handle,  snd_handle._playback_params);
    if (rc < 0) {
        printf("%s:%d:%s: ERROR [unable to set hw parameters: %s]\n",  __FILE__,  __LINE__,  __func__,  snd_strerror(rc));
        return -1;
    }

    snd_pcm_hw_params_get_period_size(snd_handle._playback_params,  &snd_handle._playback_frames,  &snd_handle._playback_dir);
    snd_pcm_hw_params_get_channels(snd_handle._playback_params, &snd_handle._play_channels);
    snd_pcm_hw_params_get_rate(snd_handle._playback_params,  &snd_handle._frequency,  &snd_handle._playback_dir);

    snd_handle._playback_buf_size = snd_handle._playback_frames * snd_handle._datablock * snd_handle._play_channels;
    snd_handle._play_buffer =(char*)malloc(snd_handle._playback_buf_size * sizeof(char));
    if (NULL == snd_handle._play_buffer) {
        printf("%s:%d:%s: ERROR [malloc _play_buffer error!]\n",  __FILE__,  __LINE__,  __func__);
        return -1;
    }

    memset(snd_handle._play_buffer, 0, snd_handle._playback_buf_size * sizeof(char));
    printf("%s:%d:%s: play_snd_pcm_init success!\n",  __FILE__,  __LINE__,  __func__);
    return 0;
}

int TestEnvProc::play_snd_pcm_deinit() {
    snd_pcm_close(snd_handle._play_handle);
    if (snd_handle._play_buffer != NULL) {
        free(snd_handle._play_buffer);
        snd_handle._play_buffer = NULL;
    }
    return 0;
}

int TestEnvProc::play_pcm(FILE * voice_file) {
    printf("INTO PLAY PCM FILE!\n", __func__);
    fseek(voice_file, 0, SEEK_SET);

    while (1) {
        int _ret = 0, read_len = 0;
        bzero(snd_handle._play_buffer,  snd_handle._playback_buf_size);
        read_len = fread(snd_handle._play_buffer,  1,  snd_handle._playback_buf_size,  voice_file);

        _ret = snd_pcm_writei(snd_handle._play_handle,  snd_handle._play_buffer,  snd_handle._playback_frames);
        if (_ret == -EPIPE) {
            printf("%s:%d:%s: play_pcm underrun occurred\n",  __FILE__,  __LINE__,  __func__);
            snd_pcm_prepare(snd_handle._play_handle);
        } else if (_ret < 0) {
            printf("%s:%d:%s: play_pcm error from write: %s\n", __FILE__,  __LINE__,  __func__ ,  snd_strerror(_ret));
        } else if (_ret != (int)snd_handle._playback_frames) {
            printf("%s:%d:%s: play_pcm short write,  write %d _playback_frames\n",  __FILE__,  __LINE__,  __func__,  _ret);
        }

        if (feof(voice_file)) {
            break;
        }
    }

    fseek(voice_file, 0, SEEK_SET);
    printf("%s:PLAY PCM FILE END!\n", __func__);

    return 0;
}

  //  #######################  asr  #########################

int TestEnvProc::asr_curl_init() {
    asr_recv.memory = new char[ASR_RESPONSE_MAX_MSG +1];
    if (nullptr == asr_recv.memory) {
        std::cout << "new asr_recv.memory err!" << std::endl;
        return -1;
    }

    asr_handle_curl._header = NULL;
    asr_handle_curl._list_par = NULL;
    asr_handle_curl._list_voi = NULL;

    //  Splicing http request header
    asr_handle_curl._header = curl_slist_append(asr_handle_curl._header, "Accept-Encoding: gzip");
    asr_handle_curl._header = curl_slist_append(asr_handle_curl._header, "Expect:");

    asr_handle_curl._header = curl_slist_append(asr_handle_curl._header,  "channel: cloudasr");
    asr_handle_curl._header = curl_slist_append(asr_handle_curl._header,  LENOVOKEY_FRAMEWORK);
    asr_handle_curl._header = curl_slist_append(asr_handle_curl._header,  SECRETKEY_FRAMEWORK);

    asr_handle_curl._list_par = curl_slist_append(asr_handle_curl._list_par, "Content-Transfer-Encoding: 8bit");
    asr_handle_curl._list_voi = curl_slist_append(asr_handle_curl._list_voi, "Content-Transfer-Encoding: binary");

    return 0;
}

int TestEnvProc::asr_curl_deinit() {
    curl_slist_free_all(asr_handle_curl._header);
    curl_slist_free_all(asr_handle_curl._list_par);
    curl_slist_free_all(asr_handle_curl._list_voi);

    delete[]  asr_recv.memory;
    return 0;
}

int TestEnvProc::asr_curl_session_init() {
    asr_handle_curl._pCurl = curl_easy_init();   //  Apply for libcurl session

    curl_easy_setopt(asr_handle_curl._pCurl, CURLOPT_HTTPHEADER, asr_handle_curl._header);  //  Add http header
    curl_easy_setopt(asr_handle_curl._pCurl, CURLOPT_URL, ASRPOSTASRURL);   //  url address
    curl_easy_setopt(asr_handle_curl._pCurl, CURLOPT_POST, 1L);   //  This operation is a post operation

    //   https request does not verify the certificate and hosts
    curl_easy_setopt(asr_handle_curl._pCurl,  CURLOPT_SSL_VERIFYPEER,  0L);
    curl_easy_setopt(asr_handle_curl._pCurl,  CURLOPT_SSL_VERIFYHOST,  0L);

    curl_easy_setopt(asr_handle_curl._pCurl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(asr_handle_curl._pCurl, CURLOPT_WRITEFUNCTION, asr_write_data);
    curl_easy_setopt(asr_handle_curl._pCurl, CURLOPT_HEADER, 0L);
    curl_easy_setopt(asr_handle_curl._pCurl, CURLOPT_VERBOSE, 0L);  //  Do not print debugging information 

    //  Do timeout processing: link timeout, received data timeout, and the timeout period is temporarily set to 15S
    curl_easy_setopt(asr_handle_curl._pCurl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(asr_handle_curl._pCurl, CURLOPT_LOW_SPEED_TIME, 15L);
    curl_easy_setopt(asr_handle_curl._pCurl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(asr_handle_curl._pCurl, CURLOPT_CONNECTTIMEOUT, 15L);

    return 0;
}

int TestEnvProc::asr_curl_session_deinit() {
    curl_easy_cleanup(asr_handle_curl._pCurl);   //  End a conversation
    return 0;
}

int TestEnvProc::asr_write_data(void *buffer,  int size,  int nmemb, void*stream) {
    memset(asr_recv.memory, 0, ASR_RESPONSE_MAX_MSG +1);
    memcpy(asr_recv.memory, buffer, (size_t)(size*nmemb));

    return size * nmemb;
}

int TestEnvProc::analysis_asr_result(char *in_buf) {
    char focus_fg[10] = {0};
    char rawtext_buf[2048] = {0};
    cJSON *_ret_parse = cJSON_Parse((char *)in_buf);
    if (!_ret_parse) {
        printf("%s:%d:%s:cJSON parse erreor! [%s]\n", __FILE__,  __LINE__,  __func__,  cJSON_GetErrorPtr());
        return static_cast<int>(music_ret::ENV_ASR_RES_NULL);
    } else {
        cJSON * raw_text = cJSON_GetObjectItem(_ret_parse, "rawText");
        if (nullptr != raw_text) {
            sprintf(rawtext_buf, "%s", raw_text->valuestring);
            std::cout << "rawText:" << rawtext_buf << std::endl;

            if (NULL != raw_text && '\0' != rawtext_buf[0]) {
                if (strlen(rawtext_buf) != 0) {
                    if (strstr(rawtext_buf,  "停止播放") != NULL) {
                        printf("%s:test 语音 停止播放！", __func__);
                        return static_cast<int>(music_ret::ENV_ASR_DISCERN_VOICE);
                    }
                } else {
                    printf("%s:test asr反回文字非停止播放！", __func__);
                    return static_cast<int>(music_ret::ENV_ASR_DISCERN_VOICE_OTHER);
                }
            } else {
                printf("%s:test asr未返回文字！", __func__);
                return static_cast<int>(music_ret::ENV_ASR_DISCERN_VOICE_ERR);   //  Voice recognition function is not turned on
            }
        } else {
            return static_cast<int>(music_ret::ENV_ASR_DISCERN_VOICE_ERR);
        }
    }

    cJSON_Delete(_ret_parse);
    return 0;
}

int TestEnvProc::asr_curl_session_proc() {
    char param_data[1024] = {0};
    char _curl_stime[14] = {0};
    long ret_code = 0;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long _stmm = (long long)tv.tv_sec*1000 + tv.tv_usec/1000;
    memset(_curl_stime, 0, sizeof(_curl_stime));
    sprintf(_curl_stime, "%lld", _stmm);

    char send_buf[(SAMPLENUM * SAMPLEBYTES * IN_CHANNELS) * 50 + 4] = {0};
    send_buf[0] = 5;
    send_buf[1] = 0;
    send_buf[2] = 0;
    send_buf[3] = 0;

    char send_buf_tmp[(SAMPLENUM * SAMPLEBYTES * IN_CHANNELS) * 200] = {0};
    //  Copy the 8-channel voice in the file
    FILE *tmp_fp = fopen("env_eight_channel_test_voice.pcm", "rb");
    if (nullptr == tmp_fp) {
        std::cout << "open env_eight_channel_test.pcm error" << std::endl;
    } else {
        fseek(tmp_fp, 0L, SEEK_END);
        unsigned int size = ftell(tmp_fp);
        rewind(tmp_fp);
        fread(send_buf_tmp , sizeof(char), size, tmp_fp);
        fclose(tmp_fp);
    }

    int offset = 0;
    int send_count = 1;
    bool endflag = false;

    char * p_send = send_buf;
    char * p_in = send_buf_tmp;

    while (4 != send_count) {
        asr_handle_curl._formpost = nullptr;
        asr_handle_curl._last_ptr = nullptr;

        memcpy(p_send + 4, p_in + offset, sizeof(send_buf) - 4);
        offset += (sizeof(send_buf) - 4);
        
        memset(param_data,0,1024);

        if (3 == send_count) {
            sprintf(param_data,  "app=com.lenovo.menu_assistant&over=1&dtp=motorola_ali&ver=1.0.9&cpts=0&city=北京市&spts=0&ssm=true&stm=%s&rvr=&ntt=wifi&aue=speex-wb;7&uid=37136136&auf=audio/L16;rate=16000&dev=lenovo.ecs.vt&sce=cmd&rsts=0&lrts=0&pidx=%ld&fpts=0&ixid=%s&vdm=all&did=270931c188c281ca&key=27802c5aa9475eacbeff13d04e4da0a8&n2lang=multi&",  _curl_stime,  send_count,  _curl_stime);
            endflag = true;
        } else {
            sprintf(param_data,  "app=com.lenovo.menu_assistant&over=0&dtp=motorola_ali&ver=1.0.9&cpts=0&city=北京市&spts=0&ssm=true&stm=%s&rvr=&ntt=wifi&aue=speex-wb;7&uid=37136136&auf=audio/L16;rate=16000&dev=lenovo.ecs.vt&sce=cmd&rsts=0&lrts=0&pidx=%ld&fpts=0&ixid=%s&vdm=all&did=270931c188c281ca&key=27802c5aa9475eacbeff13d04e4da0a8&n2lang=multi&",  _curl_stime,  send_count,  _curl_stime);
        }

        ++send_count;

        //  Upload as text
        curl_formadd(&asr_handle_curl._formpost,  &asr_handle_curl._last_ptr,  CURLFORM_COPYNAME,  "param-data",  CURLFORM_COPYCONTENTS,  param_data,  CURLFORM_CONTENTHEADER,  asr_handle_curl._list_par,  CURLFORM_CONTENTTYPE,  "text/plain; charset=UTF-8",  CURLFORM_END);
        //   Upload a file with custom file content, CURLFORM_BUFFER specifies the server file name
        curl_formadd(&asr_handle_curl._formpost,  &asr_handle_curl._last_ptr,  CURLFORM_COPYNAME,  "voice-data",  CURLFORM_BUFFER,  "voice.dat",  CURLFORM_BUFFERPTR,  send_buf,  CURLFORM_BUFFERLENGTH,  (SAMPLENUM * SAMPLEBYTES * IN_CHANNELS *50 + 4),  CURLFORM_CONTENTHEADER, asr_handle_curl._list_voi,  CURLFORM_CONTENTTYPE,  "application/i7-voice",  CURLFORM_END);
        curl_easy_setopt(asr_handle_curl._pCurl,  CURLOPT_HTTPPOST,  asr_handle_curl._formpost);

        CURLcode ret_curl = curl_easy_perform(asr_handle_curl._pCurl);
        if (CURLE_OK != ret_curl) {
            printf("%s:%s:请求失败! ret_curl:%d ret_code:%d errinfo:%s\n", __FILE__,  __func__,  ret_curl,  ret_code,  curl_easy_strerror(ret_curl));
            curl_formfree(asr_handle_curl._formpost);
            return -1;
        } else {
            printf("%s:asr server response ixid:%s\n",  __func__, _curl_stime);
            if (endflag) {
                ret_curl = curl_easy_getinfo(asr_handle_curl._pCurl,  CURLINFO_RESPONSE_CODE,  &ret_code);
                if ((CURLE_OK == ret_curl) && ret_code == 200) {
                    printf("%s:asr server msg:%s\n",  __func__,  asr_recv.memory);
                    int res_asr = analysis_asr_result(asr_recv.memory);
                    curl_formfree(asr_handle_curl._formpost);
                    return res_asr;      
                } else {
                    printf("%s:%s:获取asr响应码失败! ret_curl:%d ret_code:%d errinfo:%s\n", __FILE__,  __func__,  ret_curl,  ret_code,  curl_easy_strerror(ret_curl));
                    curl_formfree(asr_handle_curl._formpost);
                    return -1;
                }
            } else {
                curl_formfree(asr_handle_curl._formpost);
            }
        }
    }

    return 0;
}

  //  ###################   TTS   ######################

int TestEnvProc::tts_curl_init() {
    tts_recv.memory = new char[CURL_RECV_TTS_VOICE_LEN];
    if (nullptr == tts_recv.memory) {
        std::cout << "new tts_recv.memory err!" << std::endl;
        return -1;
    }

    tts_curl_session_init();
    return 0;
}

int TestEnvProc::tts_curl_deinit() {
    tts_curl_session_deinit();
    delete[] tts_recv.memory;

    return 0;
}

int TestEnvProc::tts_curl_session_init() {
    /* Apply for libcurl session */
    tts_handle_curl._pCurl = curl_easy_init();

    char param_data[1024] = {0};
    sprintf(param_data,  "text=%s&user=10123565085&speed=5&volume=5&pitch=5&audioType=4",  "你好");  //  audioType字段控制返回音频数据格式，即4为pcm,  返回的音频为16000HZ 16bit 单声道
    std::string format_txt = (char *)param_data;
    static std::string Tts_Param = encode_to_url(format_txt);

    tts_handle_curl._header = curl_slist_append(tts_handle_curl._header,  "Accept-Encoding: gzip");
    tts_handle_curl._header  = curl_slist_append(tts_handle_curl._header ,  "channel: cloudasr");
    tts_handle_curl._header  = curl_slist_append(tts_handle_curl._header ,  LENOVOKEY_FRAMEWORK);
    tts_handle_curl._header  = curl_slist_append(tts_handle_curl._header ,  SECRETKEY_FRAMEWORK);

    curl_easy_setopt(tts_handle_curl._pCurl,  CURLOPT_HTTPHEADER,  tts_handle_curl._header);
    curl_easy_setopt(tts_handle_curl._pCurl,  CURLOPT_POSTFIELDS,  Tts_Param.c_str());
    curl_easy_setopt(tts_handle_curl._pCurl,  CURLOPT_URL,  TTS_POSTURL);
    curl_easy_setopt(tts_handle_curl._pCurl,  CURLOPT_POST,  1);  //  This operation is POST
    curl_easy_setopt(tts_handle_curl._pCurl,  CURLOPT_SSL_VERIFYPEER,  0L);   //   https request does not verify the certificate and hosts
    curl_easy_setopt(tts_handle_curl._pCurl,  CURLOPT_SSL_VERIFYHOST,  0L);
    curl_easy_setopt(tts_handle_curl._pCurl,  CURLOPT_NOSIGNAL,  1L);
    curl_easy_setopt(tts_handle_curl._pCurl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(tts_handle_curl._pCurl, CURLOPT_LOW_SPEED_TIME, 15L);
    curl_easy_setopt(tts_handle_curl._pCurl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(tts_handle_curl._pCurl, CURLOPT_CONNECTTIMEOUT, 15L);

    tts_recv.size = 0;
    memset(tts_recv.memory, 0, CURL_RECV_TTS_VOICE_LEN);

    //  Write the returned result to a custom object through a callback function
    curl_easy_setopt(tts_handle_curl._pCurl,  CURLOPT_WRITEDATA,  (void *)&tts_recv);
    curl_easy_setopt(tts_handle_curl._pCurl,  CURLOPT_WRITEFUNCTION,  tts_write_data);
    curl_easy_setopt(tts_handle_curl._pCurl,  CURLOPT_HEADER, 0);
    curl_easy_setopt(tts_handle_curl._pCurl,  CURLOPT_VERBOSE, 0L);

    return 0;
}

int TestEnvProc::tts_curl_session_deinit() {
    curl_slist_free_all(tts_handle_curl._header);
    curl_easy_cleanup(tts_handle_curl._pCurl);
    return 0;
}

int TestEnvProc::tts_curl_session_proc() {
    long ret_code = 0;

    /* Perform a single curl request */
    CURLcode ret_curl = curl_easy_perform(tts_handle_curl._pCurl);
    if (ret_curl != CURLE_OK) {
        printf("%s:%d:%s: ERROR[%s]\n",  __FILE__,  __LINE__,  __func__,  curl_easy_strerror(ret_curl));
        return -1;
    }

    /* Get server response code */
    ret_curl = curl_easy_getinfo(tts_handle_curl._pCurl,  CURLINFO_RESPONSE_CODE,  &ret_code);
    if ((CURLE_OK == ret_curl) && ret_code == 200) {
        printf("%s:%d:%s: SUCCESS !\n",  __FILE__,  __LINE__,  __func__);
    } else {
        printf("%s:%d:%s: ERROR[%s] ret_code:%d\n",  __FILE__,  __LINE__,  __func__,  curl_easy_strerror(ret_curl), ret_code);
        return -1;
    }

    return 0;
}

int TestEnvProc::tts_write_data(void *buffer,  int size,  int nmemb, void*stream) {
    TEST_CALLBACK_RECV_BUF * mem = (TEST_CALLBACK_RECV_BUF*)stream;
    memcpy(&mem->memory[mem->size],  buffer,  size * nmemb);
    mem->size += size * nmemb;

    return size * nmemb;
}

  //  ##################   kugou music   ###################

int TestEnvProc::write_loading_data(void* buffer,  int size,  int nmemb,  void *stream) {
    memcpy(music_recv.memory + music_recv.size ,  buffer,  (size_t)(size * nmemb));
    music_recv.size += (size_t)(size * nmemb);
    return size * nmemb;
}

int TestEnvProc::analysis_token_id(char * token_id) {
    cJSON *ret_parse = cJSON_Parse(music_recv.memory);
    if (!ret_parse) {
        printf("%s:%d:%s: ERROR [parse url_song_recv.recv_buf error! error:%s]\n",  __FILE__,  __LINE__,  __func__,  cJSON_GetErrorPtr());
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
                    printf("%s:%d:%s: ERROR [get cms token is NULL!]\n",  __FILE__,  __LINE__,  __func__);
                    cJSON_Delete(ret_parse);
                    return -1;
                }
            } else {
                printf("%s:%d:%s: ERROR [res is NULL]\n",  __FILE__,  __LINE__,  __func__);
                cJSON_Delete(ret_parse);
                return -1;
            }
        } else {
            char msg_err[512] = {0};
            cJSON * ret_msg = cJSON_GetObjectItem(ret_parse, "msg");
            sprintf(msg_err, "%s", ret_msg->valuestring);
            printf("%s:%d:%s: ERROR [Loading status_fg:%s ,  msg_err:%s]\n",  __FILE__,  __LINE__,  __func__,  status_fg,  msg_err);
            cJSON_Delete(ret_parse);
            return -1;
        }
    } else {
        printf("%s:%d:%s: ERROR [status is NULL!]\n",  __FILE__,  __LINE__,  __func__);
        cJSON_Delete(ret_parse);
        return -1;
    }

    cJSON_Delete(ret_parse);
    return 0;
}

CURLcode TestEnvProc::Loading_Cms_Token(const char * strUrl,  int timeout, long &ret_code) {
    printf("%s:into Loading_Cms_Token!",  __func__);
    CURLcode ret_curl = CURL_LAST;
    CURL* pCURL = curl_easy_init();
    if (pCURL == NULL) {
        printf("%s:%d:%s: ERROR [Cannot use the curl functions!]\n",  __FILE__,  __LINE__,  __func__);
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(pCURL,  CURLOPT_URL,  strUrl);
    curl_easy_setopt(pCURL, CURLOPT_HEADER,  0L);
    curl_easy_setopt(pCURL,  CURLOPT_VERBOSE,  0L);
    curl_easy_setopt(pCURL,  CURLOPT_NOSIGNAL,  1L);
    curl_easy_setopt(pCURL,  CURLOPT_SSL_VERIFYPEER,  false);
    curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_TIME, 10L);
    curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 10L);

    music_recv.size = 0;
    memset(music_recv.memory, 0, sizeof(MAX_URL_SONG_MSG_LEN));
    curl_easy_setopt(pCURL,  CURLOPT_WRITEFUNCTION,  write_loading_data);

    ret_curl = curl_easy_perform(pCURL);
    if (CURLE_OK != ret_curl) {
        printf("%s:%d:%s: ERROR [curl_easy_perform is fail! error:%s]\n",  __FILE__,  __LINE__,  __func__,  curl_easy_strerror(ret_curl));
        curl_easy_cleanup(pCURL);
        return ret_curl;
    }

    ret_curl = curl_easy_getinfo(pCURL,  CURLINFO_RESPONSE_CODE,  &ret_code);
    if (CURLE_OK != ret_curl) {
        printf("%s:%d:%s: ERROR [curl_easy_getinfo is fail! error:%s]\n",  __FILE__,  __LINE__,  __func__,  curl_easy_strerror(ret_curl));
    }

    curl_easy_cleanup(pCURL);
    return ret_curl;
}

CURLcode TestEnvProc::Get_MusicList(const std::string & strUrl,  int nTimeout,  char *token_nm, long &ret_code) {
    printf("%s: into Get Music List!\n", __func__);

    CURLcode ret_curl = CURL_LAST;

    char _token_num[40] = {0};
    sprintf(_token_num,  "token:%s",  token_nm);

    CURL* pCURL = curl_easy_init();
    if (pCURL == NULL) {
        printf("%s:%d:%s: ERROR [Cannot use the curl functions!]\n",  __FILE__,  __LINE__,  __func__);
        return CURLE_FAILED_INIT;
    }

    curl_slist * _cms_headers = NULL;
    //  https
    _cms_headers = curl_slist_append(_cms_headers,  _token_num);
    _cms_headers = curl_slist_append(_cms_headers,  "XXX");  //  "mid:xxxx");

    curl_easy_setopt(pCURL,  CURLOPT_HTTPHEADER,  _cms_headers);
    curl_easy_setopt(pCURL,  CURLOPT_URL,  strUrl.c_str());
    curl_easy_setopt(pCURL, CURLOPT_HEADER, 0L);
    curl_easy_setopt(pCURL,  CURLOPT_VERBOSE,  0L);
    curl_easy_setopt(pCURL,  CURLOPT_NOSIGNAL,  1L);
    curl_easy_setopt(pCURL,  CURLOPT_TIMEOUT,  nTimeout);
    curl_easy_setopt(pCURL,  CURLOPT_NOPROGRESS,  1L);
    curl_easy_setopt(pCURL,  CURLOPT_SSL_VERIFYPEER,  false);
    curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_TIME, 10L);
    curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 10L);

    music_recv.size = 0;
    memset(music_recv.memory, 0, sizeof(MAX_URL_SONG_MSG_LEN));
    curl_easy_setopt(pCURL,  CURLOPT_WRITEFUNCTION,  write_loading_data);

    ret_curl = curl_easy_perform(pCURL);
    if (CURLE_OK != ret_curl) {
        printf("%s:%d:%s: ERROR [curl_easy_perform is fail! error:%s]\n",  __FILE__,  __LINE__,  __func__,  curl_easy_strerror(ret_curl));
        curl_slist_free_all(_cms_headers);
        curl_easy_cleanup(pCURL);
        return ret_curl;
    }

    ret_curl = curl_easy_getinfo(pCURL,  CURLINFO_RESPONSE_CODE,  &ret_code);
    if (CURLE_OK != ret_curl) {
        printf("%s:%d:%s: ERROR [curl_easy_getinfo is fail! error:%s]\n",  __FILE__,  __LINE__,  __func__,  curl_easy_strerror(ret_curl));
    }

    curl_slist_free_all(_cms_headers);
    curl_easy_cleanup(pCURL);
    return ret_curl;
}

int TestEnvProc::analysis_music_info(char * token_id) {
    printf("%s: into analysis music list result!\n",  __func__);
    cJSON *ret_parse = cJSON_Parse(music_recv.memory);
    if (!ret_parse) {
        printf("%s:%d:%s: ERROR [recv_music_list buf fail! error:%s]\n",  __FILE__,  __LINE__,  __func__, cJSON_GetErrorPtr());
        return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
    } else {
        printf("%s:%s", __func__, music_recv.memory);

        cJSON * ret_status = cJSON_GetObjectItem(ret_parse, "status");
        if (NULL != ret_status) {
            char status_fg[10] = {0};
            sprintf(status_fg, "%s", ret_status->valuestring);
            if (strcmp(status_fg, "Y") != 0) {
                char msg_err[512] = {0};
                cJSON * ret_msg = cJSON_GetObjectItem(ret_parse, "msg");
                sprintf(msg_err, "%s", ret_msg->valuestring);
                printf("%s:%d:%s: ERROR [Get_MusicList status_fg:%s ,  msg_err%s]\n", __FILE__,  __LINE__,  __func__,  status_fg,  msg_err);
                cJSON_Delete(ret_parse);
                return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
            } else {
                cJSON * ret_res = cJSON_GetObjectItem(ret_parse, "res");
                if (NULL != ret_res) {
                    cJSON * ret_rows = cJSON_GetObjectItem(ret_res, "rows");
                    if (NULL != ret_rows) {
                        cJSON * rows_list = ret_rows->child;
                        //  rows_list is NULL Exit and return, the play does not find the song list you said
                        if (NULL == rows_list) {
                            printf("%s:song list is NULL!\n", __func__);
                            cJSON_Delete(ret_parse);
                            return static_cast<int>(music_ret::NO_MUSIC_LIST_T);
                        } else {
                            int ret_music_proc_fun = music_proc(rows_list, token_id);
                            if (-1 == ret_music_proc_fun) {
                                cJSON_Delete(ret_parse);
                                return -1;
                            } else if (static_cast<int>(music_ret::MUSIC_LIST_END_T) == ret_music_proc_fun) {
                                cJSON_Delete(ret_parse);
                                return static_cast<int>(music_ret::MUSIC_LIST_END_T);
                            } else if (static_cast<int>(music_ret::GET_MUSIC_FAIL_T) == ret_music_proc_fun) {
                                cJSON_Delete(ret_parse);
                                return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
                            } else if (static_cast<int>(music_ret::GET_SONG_DATA_ERR_T) == ret_music_proc_fun) {
                                cJSON_Delete(ret_parse);
                                return static_cast<int>(music_ret::GET_SONG_DATA_ERR_T);
                            }
                        }
                    } else {
                        printf("%s:%d:%s: ERROR [Get_MusicList rows is NULL!]\n",  __FILE__,  __LINE__,   __func__);
                        cJSON_Delete(ret_parse);
                        return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
                    }
                } else {
                    printf("%s:%d:%s: ERROR [Get_MusicList res is NULL!]\n",  __FILE__,  __LINE__,  __func__);
                    cJSON_Delete(ret_parse);
                    return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
                }
            }
        } else {
            printf("%s:%d:%s: ERROR [music_list status is NULL!]\n",  __FILE__, __LINE__,  __func__);
            cJSON_Delete(ret_parse);
            return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
        }
    }

    cJSON_Delete(ret_parse);
    return 0;
}

CURLcode TestEnvProc::Get_SongUrl(const std::string &strUrl,  int nTimeout,  char *token_nm, long &ret_code) {
    printf("%s: into Get Song URL!\n",  __func__);

    CURLcode ret_curl = CURL_LAST;

    char _token_num[40] = {0};
    sprintf(_token_num,  "token:%s",  token_nm);

    CURL* pCURL = curl_easy_init();
    if (pCURL == NULL) {
        printf("%s:%d:%s: ERROR [Cannot use the curl functions!]\n",  __FILE__,  __LINE__,  __func__);
        return CURLE_FAILED_INIT;
    }

    curl_slist * _cms_headers = NULL;

    _cms_headers = curl_slist_append(_cms_headers,  _token_num);
    _cms_headers = curl_slist_append(_cms_headers,  "XXX");  //  "mid:xxxx");

    curl_easy_setopt(pCURL,  CURLOPT_HTTPHEADER,  _cms_headers);
    curl_easy_setopt(pCURL,  CURLOPT_URL,  strUrl.c_str());
    curl_easy_setopt(pCURL, CURLOPT_HEADER, 0L);
    curl_easy_setopt(pCURL,  CURLOPT_VERBOSE,  0L);
    curl_easy_setopt(pCURL,  CURLOPT_NOSIGNAL,  1L);
    curl_easy_setopt(pCURL,  CURLOPT_TIMEOUT,  nTimeout);
    curl_easy_setopt(pCURL,  CURLOPT_NOPROGRESS,  1L);
    curl_easy_setopt(pCURL,  CURLOPT_SSL_VERIFYPEER,  false);
    curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_TIME, 10L);
    curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 10L);

    music_recv.size = 0;
    memset(music_recv.memory, 0, sizeof(MAX_URL_SONG_MSG_LEN));

    curl_easy_setopt(pCURL,  CURLOPT_WRITEFUNCTION,  write_loading_data);

    ret_curl = curl_easy_perform(pCURL);
    if (CURLE_OK != ret_curl) {
        curl_slist_free_all(_cms_headers);
        curl_easy_cleanup(pCURL);
        printf("%s:%d:%s: ERROR [curl_easy_perform is fail! error:%s]\n",  __FILE__,  __LINE__,   __func__,  curl_easy_strerror(ret_curl));
        return ret_curl;
    }

    ret_curl = curl_easy_getinfo(pCURL,  CURLINFO_RESPONSE_CODE,  &ret_code);
    if (CURLE_OK != ret_curl) {
        printf("%s:%d:%s:curl_easy_getinfo is fail! error:%s\n",  __FILE__,  __LINE__,  __func__,  curl_easy_strerror(ret_curl));
    }

    curl_slist_free_all(_cms_headers);
    curl_easy_cleanup(pCURL);

    return ret_curl;
}

CURLcode TestEnvProc::Get_SongData(char * strUrl,  int nTimeout, long &ret_code, char * Redirect_url, int Redirect_url_len, bool &get_redirect_url_status) {
    printf("%s:into Get Song mp3 Data!\n", __func__);

    CURLcode ret_curl;
    memset(Redirect_url, 0, Redirect_url_len);
    get_redirect_url_status = false;

    CURL* pCURL = curl_easy_init();
    if (pCURL == NULL) {
        printf("%s:%d:%s: ERROR [Cannot use the curl functions!]\n",  __FILE__,  __LINE__,  __func__);
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(pCURL,  CURLOPT_URL,  strUrl);
    curl_easy_setopt(pCURL, CURLOPT_HEADER, 0L);
    curl_easy_setopt(pCURL,  CURLOPT_VERBOSE,  0L);
    curl_easy_setopt(pCURL,  CURLOPT_NOSIGNAL,  1L);
    curl_easy_setopt(pCURL,  CURLOPT_TIMEOUT,  nTimeout);
    curl_easy_setopt(pCURL,  CURLOPT_NOPROGRESS,  1L);
    curl_easy_setopt(pCURL,  CURLOPT_FOLLOWLOCATION,  1L);
    curl_easy_setopt(pCURL,  CURLOPT_SSL_VERIFYPEER,  false);
    curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(pCURL, CURLOPT_LOW_SPEED_TIME, 15L);
    curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 15L);

    music_recv.size = 0;
    memset(music_recv.memory, 0, sizeof(MAX_URL_SONG_MSG_LEN));
    curl_easy_setopt(pCURL,  CURLOPT_WRITEFUNCTION,  write_loading_data);

    ret_curl = curl_easy_perform(pCURL);
    if (CURLE_OK != ret_curl) {
        curl_easy_cleanup(pCURL);
        printf("%s:%d:%s: ERROR [curl_easy_perform is fail! error:%s]\n",  __FILE__,  __LINE__,  __func__,  curl_easy_strerror(ret_curl));
        return ret_curl;
    }

    ret_curl = curl_easy_getinfo(pCURL,  CURLINFO_RESPONSE_CODE,  &ret_code);
    if (CURLE_OK != ret_curl) {
        printf("%s:%d:%s: ERROR [curl_easy_getinfo is fail! error:%s]\n",  __FILE__,  __LINE__,  __func__,  curl_easy_strerror(ret_curl));
    }

    if (200 != ret_code) {
        char* url = NULL;
        curl_easy_getinfo(pCURL,  CURLINFO_REDIRECT_URL,  &url);
        if (url) {
            get_redirect_url_status = true;
            printf("Redirect url = %d\n", url);
            std::string str_url(url);
            memcpy(Redirect_url, str_url.c_str(), str_url.length());
            printf("get redirect url success!\n");
        }
    }

    curl_easy_cleanup(pCURL);

    printf("%s:get song data success!, song size:[%d]\n", __func__, music_recv.size);

    return ret_curl;
}

int TestEnvProc::music_proc(cJSON * &rows_list, char * &token_id) {
    printf("%s: into play music proc!\n",  __func__);

    bool get_music_data_status = false;
    int  get_music_data_count = 0;

    size_t len = 0;
    char params[256] = {0} , status_fg[10] = {0},  msg_err[512] = {0};
    char * songId = NULL,  *albumId = NULL ,  *songUrl = NULL;
    std::string song_url_str;
    cJSON * bf_rows_list = rows_list;
    long ret_code = 0;
    CURLcode ret;

    cJSON * ret_music = NULL , *ret_status = NULL ,  *ret_msg = NULL , *ret_res = NULL;

    while (NULL != rows_list) {
        songId = NULL;
        albumId = NULL;
        songUrl = NULL;
        song_url_str.clear();
        memset(params, 0, sizeof(params));

        songId = cJSON_GetObjectItem(rows_list, "songId")->valuestring;
        albumId = cJSON_GetObjectItem(rows_list, "albumId")->valuestring;

        if (strlen(albumId) == 0) {
            sprintf(params, "XXX%s",  songId);  //  "https://xxx.xx.com.cn/musicSearch/audio/getSong?songId=%s&albumId=0&cpId=kg",  songId);
        } else {
            sprintf(params,  "XXX%s %s",  songId,  albumId);  //  "https://xxx.xxx.com.cn/musicSearch/audio/getSong?songId=%s&albumId=%s&cpId=kg",  songId,  albumId);
        }

        song_url_str = params;

        ret = Get_SongUrl(song_url_str, 300, token_id, ret_code);

        if (CURLE_OK == ret && 200 == ret_code) {
            if (NULL != ret_music) {
                cJSON_Delete(ret_music);
                ret_music = NULL;
            }

            ret_music = cJSON_Parse(music_recv.memory);
            if (!ret_music) {
                printf("%s:%d:%s: ERROR [Get_SongUrl rec_buf error:%s]\n",  __FILE__,  __LINE__,  __func__,  cJSON_GetErrorPtr());
                return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
            } else {
                ret_status = cJSON_GetObjectItem(ret_music, "status");
                if (NULL != ret_status) {
                    memset(status_fg, 0, sizeof(status_fg));
                    sprintf(status_fg, "%s", ret_status->valuestring);
                    if (strcmp(status_fg, "Y") != 0) {
                        ret_msg = cJSON_GetObjectItem(ret_music, "msg");
                        memset(msg_err, 0, sizeof(msg_err));
                        sprintf(msg_err, "%s", ret_msg->valuestring);
                        printf("%s:%d:%s: ERROR [Get_SongUrl status_fg:%s ,  msg_err:%s]\n",  __FILE__,  __LINE__,  __func__,  status_fg,  msg_err);
                        cJSON_Delete(ret_music);
                        return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
                    } else {
                        ret_res = cJSON_GetObjectItem(ret_music, "res");
                        if (NULL != ret_res) {
                            songUrl = cJSON_GetObjectItem(ret_res, "songUrl")->valuestring;
                            printf("%s:songurl:%s\n",  __func__,  songUrl);
                        } else {
                            printf("%s:%d:%s: ERROR [songurl is NULL!]\n",  __FILE__,  __LINE__,  __func__);

                            cJSON_Delete(ret_music);
                            return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
                        }
                    }
                } else {
                    printf("%s:%d:%s: ERROR [Get_SongUrl recv_buf status is NULL!]\n",  __FILE__,  __LINE__,  __func__);
                    cJSON_Delete(ret_music);
                    return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
                }
            }
        } else {
            printf("%s:%d:%s: ERROR [Get_SongUrl fail! error:%s ret_code:%d]\n",  __FILE__,  __LINE__,  __func__,  curl_easy_strerror(ret), ret_code);

            if (CURLE_OK == ret) {
                return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
            } else {
                return -1;
            }
        }

        //  Get song data according to songUrl
        get_music_data_count = 0;
        get_music_data_status = true;   //  true:Play song
        char Redirect_url[1024] = {0};
        char in_url[1024] = {0};
        bool get_redirect_url_status = false;
        while (get_music_data_status) {
            ++get_music_data_count;
            printf("%s:%d:%s:请求歌曲数据的次数：%d\n",  __FILE__,  __LINE__,  __func__,  get_music_data_count);
            ret = Get_SongData(songUrl, 300, ret_code, Redirect_url, sizeof(Redirect_url), get_redirect_url_status);

            if (CURLE_OK == ret && 200 == ret_code) {
                if (0 == music_recv.size) {
                    printf("%s:%d:%s: ERROR [SongData is NULL!]\n",  __FILE__,  __LINE__,  __func__);
                    cJSON_Delete(ret_music);
                    return static_cast<int>(music_ret::GET_SONG_DATA_ERR_T);
                }

                get_music_data_status = false;
                printf("get music success!\n");
                cJSON_Delete(ret_music);
                return 0;
            } else {
                printf("%s:%d:%s: ERROR [Get_SongData fail! error:%s ret_code:%d]\n",  __FILE__,  __LINE__,  __func__,  curl_easy_strerror(ret), ret_code);

                if (get_redirect_url_status) {
                    memcpy(in_url, Redirect_url, sizeof(Redirect_url));
                    songUrl = in_url;
                    printf("重新获取的url给songUrl赋值！");
                }

                if (2 <= get_music_data_count) {
                    get_music_data_status = false;

                    printf("%s:%d:%s:请求歌曲数据的次数：%d, 已经请求两次，结束歌曲数据的请求！\n",  __FILE__,  __LINE__,  __func__,  get_music_data_count);

                    cJSON_Delete(ret_music);
                    if (CURLE_OK == ret) {
                        return static_cast<int>(music_ret::GET_SONG_DATA_ERR_T);
                    } else {
                        return -1;
                    }
                }
            }
        }

        rows_list = rows_list->next;
        if (NULL == rows_list) {
            printf("%s:音乐列表播放结束!",  __func__);
            cJSON_Delete(ret_music);
            return 0;
        }
    }

    return 0;
}

int TestEnvProc::kugou_music_init() {
    music_recv.memory = new char[MAX_URL_SONG_MSG_LEN];
    if (nullptr == music_recv.memory) {
        std::cout << "new music.recv err!" << std::endl;
        return -1;
    }

    return 0;
}

int TestEnvProc::kugou_music_deinit() {
    delete[] music_recv.memory;
    return 0;
}

int TestEnvProc::kugou_music_proc() {
    long ret_code = 0;
    char token_id[36] = {0} ,  music_list[512] = {0};
    CURLcode ret = CURL_LAST;

    ret = Loading_Cms_Token(TOKEN_URL, 1000, ret_code);
    if (CURLE_OK == ret && 200 == ret_code) {
        if (-1 == analysis_token_id(token_id)) {
            return -1;
        }
    } else {
        printf("%s:%d:%s: ERROR [ret:%d ret_code:%d]\n",  __FILE__,  __LINE__,  __func__,  ret,  ret_code);
        printf("%s:%d:%s: ERROR [Loading_Cms_Token fail! error:%s]\n", __FILE__,  __LINE__,   __func__,  curl_easy_strerror(ret));

        if (CURLE_OK == ret) {
            printf("非网络问题就是获取失败\n");
        } else {
            printf("网络问题\n");
        }

        return -1;
    }

    std::string musiclist_urlstr;   //  Music list url
    sprintf(music_list, MUSIC_LIST_URL_ARG, "忘情水", "刘德华");

    std::string music_list_str = music_list;
    musiclist_urlstr = encode_to_url(music_list_str);

    ret = Get_MusicList(musiclist_urlstr, 300, token_id, ret_code);
    if (CURLE_OK == ret && 200 == ret_code) {
           //  解析结果
        int ret_analysis = analysis_music_info(token_id);
        if (-1 == ret_analysis) {
            return -1;
        } else if (static_cast<int>(music_ret::NO_MUSIC_LIST_T) == ret_analysis) {
            return static_cast<int>(music_ret::NO_MUSIC_LIST_T);
        } else if (static_cast<int>(music_ret::MUSIC_LIST_END_T) == ret_analysis) {
            return static_cast<int>(music_ret::MUSIC_LIST_END_T);
        } else if (static_cast<int>(music_ret::GET_MUSIC_FAIL_T) == ret_analysis) {
            return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
        } else if (static_cast<int>(music_ret::GET_SONG_DATA_ERR_T) == ret_analysis) {
            return static_cast<int>(music_ret::GET_SONG_DATA_ERR_T);
        }
    } else {
        printf("%s:%d:%s: ERROR [ret:%d ret_code:%d]\n",  __FILE__,  __LINE__,  __func__,  ret,  ret_code);
        printf("%s:%d:%s: ERROR [Get_musiclist fail! error:%s]\n",  __FILE__,  __LINE__,  __func__,  curl_easy_strerror(ret));

        if (CURLE_OK == ret) {
            return static_cast<int>(music_ret::GET_MUSIC_FAIL_T);
        } else {
            return -1;
        }
    }

    return 0;
}

//   end file
