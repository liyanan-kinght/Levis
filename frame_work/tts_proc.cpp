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
#include "tts_proc.h"

#define __DUMP__ 0  //  Set the value to 1，Only when you want a specific voice, convert the text to voice through tts and save the pcm file.
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

TTS_PROC_STATUS ttsthreadmod;

TtsProc::TtsProc() { }

TtsProc::~TtsProc() { }

int TtsProc::Init() {
#if __SEND_LOG_TO_SERVER__
    tts_info_stru.sendlogobj = new SendLogProc();
    if (NULL == tts_info_stru.sendlogobj) {
        printf("%s:%d:%s: ERROR [Client send log obj malloc err!]\n", __FILE__, __LINE__, __func__);
        tts_info_stru.client_send_status = false;
    } else {
        tts_info_stru.client_send_status = true;
    }

    if (-1 == tts_info_stru.sendlogobj->Init()) {
        printf("%s:%d:%s: ERROR [send log Init err!]\n", __FILE__, __LINE__, __func__);
        tts_info_stru.client_send_status = false;
    } else {
        tts_info_stru.client_send_status = true;
    }
#endif

    if (-1 == memory_init()) {
        return -1;
    }

    mutex_and_cond_init();

    if (-1 == init_thread()) {
        return -1;
    }

    return 0;
}

int TtsProc::DeInit() {
    DEBUG("INTO TTS PROC DEINIT");
    pthread_mutex_lock(&tts_info_stru.mutex_cond_stru.txt_mutex);
    pthread_cond_broadcast(&tts_info_stru.mutex_cond_stru.voice_cond);
    pthread_mutex_unlock(&tts_info_stru.mutex_cond_stru.txt_mutex);

    deinit_thread();
    memory_deinit();
    mutex_and_cond_deinit();

#if __SEND_LOG_TO_SERVER__
    if (NULL != tts_info_stru.sendlogobj) {
        tts_info_stru.sendlogobj->DeInit();
        delete tts_info_stru.sendlogobj;
    }
#endif
    printf("%s:%d:%s: tts deinit success!\n", __FILE__, __LINE__, __func__);
    return 0;
}

int TtsProc::init_thread() {
    int ret = 0;
    ret = pthread_create(&tts_info_stru.tts_voice_thread_id, NULL, TtsProcess, this);
    if (0 != ret) {
        printf("%s:%d:%s: ERROR[thread create error!]\n", __FILE__, __LINE__, __func__);
        if (tts_info_stru.client_send_status)
            SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "%s: ERROR[thread create error!]\n", __func__);
        return -1;
    }

    return 0;
}

int TtsProc::deinit_thread() {
    DEBUG("INTO THREAD DEINIT");
    if (tts_info_stru.client_send_status)
        SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "INTO THREAD DEINIT");

    pthread_join(tts_info_stru.tts_voice_thread_id, NULL);
    printf("%s:%d:%s: tts thread deinit success!\n", __FILE__, __LINE__, __func__);

    if (tts_info_stru.client_send_status)
        SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "%s: tts thread deinit success!\n", __func__);

    return 0;
}

int TtsProc::mutex_and_cond_init() {
    pthread_mutex_init(&tts_info_stru.mutex_cond_stru.txt_mutex, NULL);
    pthread_cond_init(&tts_info_stru.mutex_cond_stru.voice_cond, NULL);
    printf("%s:%d:%s: tts mutex and cond init success!\n", __FILE__, __LINE__, __func__);

    if (tts_info_stru.client_send_status)
        SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "%s: tts mutex and cond init success!\n", __func__);

    return 0;
}

int TtsProc::mutex_and_cond_deinit() {
    pthread_mutex_destroy(&tts_info_stru.mutex_cond_stru.txt_mutex);
    pthread_cond_destroy(&tts_info_stru.mutex_cond_stru.voice_cond);
    printf("%s:%d:%s: tts mutex and cond deinit success!\n", __FILE__, __LINE__, __func__);

    if (tts_info_stru.client_send_status)
        SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "%s: tts mutex and cond deinit success!\n", __func__);

    return 0;
}

int TtsProc::pause() {
    tts_info_stru.tts_pause = true;

    if (tts_info_stru.tts_voice_action) {
        if (tts_info_stru.client_send_status) {
            SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "停止之前tts voice任务！");
        } else {
            INFO("停止之前tts voice任务！");
        }
    } else {
        tts_info_stru.tts_pause = false;
        return -1;
    }

    return 0;
}

int TtsProc::set_tts_info(const char *ttsTxt, int len, TTS_RES callback, void *arg) {
    if (tts_info_stru.client_send_status) {
        SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "%s: frame_work get tts lock\n",  __func__);
    } else {
        printf("%s:%d:%s: frame_work get tts lock\n", __FILE__, __LINE__, __func__);
    }

    pthread_mutex_lock(&tts_info_stru.mutex_cond_stru.txt_mutex);

    // copy tts txt string
    memcpy(tts_info_stru.tts_txt_record_buf, ttsTxt, len);
    tts_info_stru.frame_work_callback.arg = arg;
    tts_info_stru.frame_work_callback.get_tts_res = callback;
    tts_info_stru.tts_voice_action = true;

    pthread_cond_signal(&tts_info_stru.mutex_cond_stru.voice_cond);
    pthread_mutex_unlock(&tts_info_stru.mutex_cond_stru.txt_mutex);

    if (tts_info_stru.client_send_status) {
        SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "%s: frame_woek get tts unlock\n",  __func__);
    } else {
        printf("%s:%d:%s: frame_woek get tts unlock\n", __FILE__, __LINE__, __func__);
    }

    return 0;
}


void * TtsProc::TtsProcess(void *arg) {
    printf("%s:%d:%s:###TTS VOICE THREAD RUN!###\n", __FILE__, __LINE__, __func__);

    TtsProc *obj = reinterpret_cast<TtsProc *>(arg);
    if (ttsthreadmod.tts_thread_stop) {
        if (obj->tts_info_stru.client_send_status) {
            SENDLOG_CLI_OBJ_PTR(obj->tts_info_stru.sendlogobj, DEBUG, "###TTS VOICE THREAD END!###\n");
        }
        printf("%s:%d:%s:###tts voice end!###\n", __FILE__, __LINE__, __func__);
        pthread_exit(NULL);
    }

    if (obj->tts_info_stru.client_send_status)
        SENDLOG_CLI_OBJ_PTR(obj->tts_info_stru.sendlogobj, DEBUG, "###TTS VOICE THREAD RUN!###\n");

    pthread_mutex_lock(&obj->tts_info_stru.mutex_cond_stru.txt_mutex);

    while (!ttsthreadmod.tts_thread_stop) {
        while (!ttsthreadmod.tts_thread_stop && false == obj->tts_info_stru.tts_voice_action) {
            if (obj->tts_info_stru.client_send_status) {
                SENDLOG_CLI_OBJ_PTR(obj->tts_info_stru.sendlogobj, DEBUG, "wait tts txt !");
            } else {
                DEBUG("wait tts txt !");
            }

            pthread_cond_wait(&obj->tts_info_stru.mutex_cond_stru.voice_cond, &obj->tts_info_stru.mutex_cond_stru.txt_mutex);

            if (obj->tts_info_stru.client_send_status) {
                SENDLOG_CLI_OBJ_PTR(obj->tts_info_stru.sendlogobj, DEBUG, "wait tts txt success!");
            } else {
                DEBUG("wait tts txt success!");
            }
        }

        if (ttsthreadmod.tts_thread_stop) {
            if (obj->tts_info_stru.client_send_status) {
                SENDLOG_CLI_OBJ_PTR(obj->tts_info_stru.sendlogobj, DEBUG, "%s: tts_thread_stop is true!", __func__);
            } else {
                INFO("%s: tts_thread_stop is true!", __func__);
            }
            break;
        }

        if (0 == obj->get_tts_voice(obj->tts_info_stru.tts_txt_record_buf)) {
            if (!obj->tts_info_stru.tts_pause) {
                memset(obj->tts_info_stru.tts_voice_2_channel_buf, 0, TTS_VOICE_LEN);
                memcpy(obj->tts_info_stru.tts_voice_2_channel_buf, obj->tts_info_stru.tts_url_callback_buf.memory, obj->tts_info_stru.tts_url_callback_buf.size);
            }

#if __DUMP__
            FILE * tmp_fp = fopen("tts_dump.pcm", "wb");
            if (NULL == tmp_fp) {
                printf("%s:%d:%s: ERROR [tts_dump.pcm file open fail!]\n", __FILE__, __LINE__, __func__);
            } else {
                // fwrite(obj->tts_info_stru.tts_voice_2_channel_buf, sizeof(char), obj->tts_info_stru.tts_url_callback_buf.size * 2, tmp_fp);
                fwrite(obj->tts_info_stru.tts_voice_2_channel_buf, sizeof(char), obj->tts_info_stru.tts_url_callback_buf.size, tmp_fp);
                fclose(tmp_fp);
                tmp_fp = NULL;
                exit(0);
            }
#endif

            if (!obj->tts_info_stru.tts_pause) {
                obj->tts_info_stru.frame_work_callback.get_tts_res(obj->tts_info_stru.tts_voice_2_channel_buf,
                                                                    obj->tts_info_stru.tts_url_callback_buf.size,
                                                                    obj->tts_info_stru.frame_work_callback.arg);
                obj->tts_info_stru.tts_pause = false;

                if (obj->tts_info_stru.client_send_status) {
                    SENDLOG_CLI_OBJ_PTR(obj->tts_info_stru.sendlogobj, DEBUG, "tts voice拷贝数据到frame_work");
                } else {
                    DEBUG("tts voice拷贝数据到frame_work");
                }
            } else {
                obj->tts_info_stru.tts_pause = false;
            }
        } else {
            if (!obj->tts_info_stru.tts_pause) {
                obj->tts_info_stru.frame_work_callback.get_tts_res(NULL, 0, obj->tts_info_stru.frame_work_callback.arg);
                obj->tts_info_stru.tts_pause = false;

                if (obj->tts_info_stru.client_send_status) {
                    SENDLOG_CLI_OBJ_PTR(obj->tts_info_stru.sendlogobj, DEBUG, "获取tts 音频数据时错误！");
                }

                DEBUG("获取tts 音频数据时错误！");
            } else {
                obj->tts_info_stru.tts_pause = false;
            }
        }

        obj->tts_info_stru.tts_voice_action = false;
    }

    pthread_mutex_unlock(&obj->tts_info_stru.mutex_cond_stru.txt_mutex);

    if (obj->tts_info_stru.client_send_status) {
        SENDLOG_CLI_OBJ_PTR(obj->tts_info_stru.sendlogobj, DEBUG, "###tts voice end!###");
    } else {
        printf("%s:%d:%s:###tts voice end!###\n", __FILE__, __LINE__, __func__);
    }

    pthread_exit(NULL);
}

int TtsProc::write_ttsdata(void * buffer, int size, int nmemb, void *userp) {
    TTS_CALLBACK_MEMORY *mem = reinterpret_cast<TTS_CALLBACK_MEMORY*>(userp);
    memcpy(&mem->memory[mem->size], buffer, size * nmemb);
    mem->size += size * nmemb;
    return size * nmemb;
}

int TtsProc::get_tts_voice(char *tts_curldata) {
    printf("%s:%d:%s into get tts voice!\n", __FILE__, __LINE__, __func__);
    if (tts_info_stru.client_send_status)
        SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, " %s into get tts voice!", __func__);

    long ret_code = 0;
    char param_data[1024];
    tts_info_stru.tts_curl_stru._headers = NULL;

    snprintf(param_data, sizeof(param_data), "text=%s&user=10123565085&speed=5&volume=5&pitch=5&audioType=4", tts_curldata);
    std::string format_txt = reinterpret_cast<char *>(param_data);
    std::string Tts_Param = encode_to_url(format_txt);

    tts_info_stru.tts_curl_stru.tts_curl = curl_easy_init();

    tts_info_stru.tts_curl_stru._headers = curl_slist_append(tts_info_stru.tts_curl_stru._headers, "Accept-Encoding: gzip");
    tts_info_stru.tts_curl_stru._headers = curl_slist_append(tts_info_stru.tts_curl_stru._headers, "channel: cloudasr");
    tts_info_stru.tts_curl_stru._headers = curl_slist_append(tts_info_stru.tts_curl_stru._headers, LENOVOKEY_FRAMEWORK);
    tts_info_stru.tts_curl_stru._headers = curl_slist_append(tts_info_stru.tts_curl_stru._headers, SECRETKEY_FRAMEWORK);

    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_HTTPHEADER, tts_info_stru.tts_curl_stru._headers);
    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_POSTFIELDS, Tts_Param.c_str());
    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_URL, TTS_POSTURL);
    // curl_easy_setopt(tts_curl, CURLOPT_URL, TTS_GETURL);
    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // curl_easy_setopt(tts_curl, CURLOPT_CONNECTTIMEOUT, 1L);
    // curl_easy_setopt(tts_curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_NOSIGNAL, 1L);
    // curl_easy_setopt(tts_curl, CURLOPT_NOPROGRESS, 1L);

    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_LOW_SPEED_TIME, 15L);
    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_CONNECTTIMEOUT, 15L);

    tts_info_stru.tts_url_callback_buf.size = 0;
    memset(tts_info_stru.tts_url_callback_buf.memory, 0, CURL_RECV_TTS_VOICE_LEN);

    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_WRITEDATA, reinterpret_cast<void *>(&tts_info_stru.tts_url_callback_buf));
    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_WRITEFUNCTION, write_ttsdata);
    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_HEADER, 0L);
    curl_easy_setopt(tts_info_stru.tts_curl_stru.tts_curl, CURLOPT_VERBOSE, 0L);

    if (!tts_info_stru.tts_pause) {
        if (!tts_info_stru.tts_pause) {
            CURLcode ret_curl = curl_easy_perform(tts_info_stru.tts_curl_stru.tts_curl);
            if (ret_curl != CURLE_OK) {
                printf("%s:%d:%s: ERROR[%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl));
                if (tts_info_stru.client_send_status)
                    SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "%s: ERROR[%s]\n", __func__, curl_easy_strerror(ret_curl));
                curl_slist_free_all(tts_info_stru.tts_curl_stru._headers);
                curl_easy_cleanup(tts_info_stru.tts_curl_stru.tts_curl);
                return -1;
            }
        }

        if (!tts_info_stru.tts_pause) {
            CURLcode ret_curl = curl_easy_getinfo(tts_info_stru.tts_curl_stru.tts_curl, CURLINFO_RESPONSE_CODE, &ret_code);
            if ((CURLE_OK == ret_curl) && ret_code == 200) {
                DEBUG("%s: url recv tts voice size :%d", __func__, tts_info_stru.tts_url_callback_buf.size);
                if (tts_info_stru.client_send_status)
                    SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "%s: url recv tts voice size :%d", __func__, tts_info_stru.tts_url_callback_buf.size);
            } else {
                printf("%s:%d:%s: ERROR[%s] ret_code:%d\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl), ret_code);
                if (tts_info_stru.client_send_status)
                    SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "%s: ERROR[%s] ret_code:%d\n",  __func__, curl_easy_strerror(ret_curl), ret_code);
                curl_slist_free_all(tts_info_stru.tts_curl_stru._headers);
                curl_easy_cleanup(tts_info_stru.tts_curl_stru.tts_curl);
                return -1;
            }
        }
    }

    curl_slist_free_all(tts_info_stru.tts_curl_stru._headers);
    curl_easy_cleanup(tts_info_stru.tts_curl_stru.tts_curl);
    printf("%s:%d:%s get tts voice end! tts len:%d\n", __FILE__, __LINE__, __func__, tts_info_stru.tts_url_callback_buf.size);

    if (tts_info_stru.client_send_status)
        SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "%s get tts voice end! tts len:%d\n", __func__, tts_info_stru.tts_url_callback_buf.size);

    return 0;
}

int TtsProc::memory_init() {
    tts_info_stru.tts_url_callback_buf.memory = reinterpret_cast<char *>(malloc(CURL_RECV_TTS_VOICE_LEN));
    if (NULL == tts_info_stru.tts_url_callback_buf.memory) {
        printf("%s:%d:%s: ERROR[fail malloc callback recv voice buf!]\n", __FILE__, __LINE__, __func__);
        if (tts_info_stru.client_send_status)
            SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "%s: ERROR[fail malloc callback recv voice buf!]\n", __func__);
        return -1;
    }
    return 0;
}

int TtsProc::memory_deinit() {
    if (tts_info_stru.tts_url_callback_buf.memory) {
        free(tts_info_stru.tts_url_callback_buf.memory);
        tts_info_stru.tts_url_callback_buf.memory = NULL;
    }

    printf("%s:%d:%s: tts memory deinit success!\n", __FILE__, __LINE__, __func__);

    if (tts_info_stru.client_send_status)
        SENDLOG_CLI_OBJ_PTR(tts_info_stru.sendlogobj, DEBUG, "%s: tts memory deinit success!\n", __func__);

    return 0;
}

std::string TtsProc::encode_to_url(std::string &str_source) {
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
        } else if ((c < '0' && c != '-' && c != '.' && c != '/' && c != '&')
                    || (c < 'A' && c > '9' && c != ':' && c!= '?' && c != '=')
                    || (c > 'Z' && c < 'a' && c != '_') || (c > 'z')) {
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
    out_str = reinterpret_cast<char *>(start);
    free(start);
    start = to = NULL;

    return out_str;
}
