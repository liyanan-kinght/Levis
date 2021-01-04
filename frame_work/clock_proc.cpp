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

#include "clock_proc.h"

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

using std::cout;
using std::cin;
using std::endl;
using std::string;

CLOCK_VOICE_STATUS clockvoicethreadmod;

int ClockVoiceProc::Init() {
#if __SEND_LOG_TO_SERVER__
    clock_voice_stru.sendlogobj = new SendLogProc();
    if (NULL == clock_voice_stru.sendlogobj) {
        printf("%s:%d:%s: ERROR [thread malloc err!]\n", __FILE__, __LINE__, __func__);
        clock_voice_stru.client_send_log_status = false;
    } else {
        clock_voice_stru.client_send_log_status = true;
    }

    if (-1 == clock_voice_stru.sendlogobj->Init()) {
        printf("%s:%d:%s: ERROR [send log Init err!]\n", __FILE__, __LINE__, __func__);
        clock_voice_stru.client_send_log_status = false;
    } else {
        clock_voice_stru.client_send_log_status = true;
    }
#endif

    if (-1 == memory_init()) {
        return -1;
    }

    mutex_and_cond_init();

    if (-1 == init_thread()) {
        return -1;
    }

    printf("%s:%d:%s:clock voice init sucess!###\n", __FILE__, __LINE__, __func__);

    if (clock_voice_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s:clock voice init sucess!\n");

    return 0;
}

int ClockVoiceProc::DeInit() {
    DEBUG("INTO CLOCK VOICE DEINIT");

    if (clock_voice_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "INTO CLOCK VOICE DEINIT");

    pthread_mutex_lock(&clock_voice_stru.mutex_cond_stru.txt_mutex);
    pthread_cond_broadcast(&clock_voice_stru.mutex_cond_stru.voice_cond);
    pthread_mutex_unlock(&clock_voice_stru.mutex_cond_stru.txt_mutex);

    deinit_thread();
    memory_deinit();
    mutex_and_cond_deinit();

#if __SEND_LOG_TO_SERVER__
    if (NULL != clock_voice_stru.sendlogobj) {
        clock_voice_stru.sendlogobj->DeInit();
        delete clock_voice_stru.sendlogobj;
    }
#endif

    printf("%s:%d:%s: clock voice deinit success!\n", __FILE__, __LINE__, __func__);
    return 0;
}

int ClockVoiceProc::init_thread() {
    int ret = 0;
    ret = pthread_create(&clock_voice_stru.clock_voice_thr_id, NULL, ClockTts, this);
    if (0 != ret) {
        printf("%s:%d:%s: ERROR[thread create error!]\n", __FILE__, __LINE__, __func__);
        if (clock_voice_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s: ERROR[thread create error!]\n", __func__);
        return -1;
    }

    printf("%s:%d:%s:###clock voice thread init sucess!###\n", __FILE__, __LINE__, __func__);

    if (clock_voice_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s:###clock voice thread init sucess!###\n", __func__);

    return 0;
}

int ClockVoiceProc::deinit_thread() {
    DEBUG("INTO CLOCK VOICE THREAD DEINIT");

    if (clock_voice_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "INTO CLOCK VOICE THREAD DEINIT");

    pthread_join(clock_voice_stru.clock_voice_thr_id, NULL);
    printf("%s:%d:%s: clock voice thread deinit success!\n", __FILE__, __LINE__, __func__);

    if (clock_voice_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s: clock voice thread deinit success!\n", __func__);

    return 0;
}

int ClockVoiceProc::mutex_and_cond_init() {
    pthread_mutex_init(&clock_voice_stru.mutex_cond_stru.txt_mutex, NULL);
    pthread_cond_init(&clock_voice_stru.mutex_cond_stru.voice_cond, NULL);
    printf("%s:%d:%s: clock voice mutex and cond init success!\n", __FILE__, __LINE__, __func__);

    if (clock_voice_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s: clock voice mutex and cond init success!\n", __func__);

    return 0;
}

int ClockVoiceProc::mutex_and_cond_deinit() {
    pthread_mutex_destroy(&clock_voice_stru.mutex_cond_stru.txt_mutex);
    pthread_cond_destroy(&clock_voice_stru.mutex_cond_stru.voice_cond);
    printf("%s:%d:%s: clock voice mutex and cond deinit success!\n", __FILE__, __LINE__, __func__);

    if (clock_voice_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s: clock voice mutex and cond deinit success!\n", __func__);

    return 0;
}

int ClockVoiceProc::memory_init() {
    clock_voice_stru.tts_cache_stru.memory = reinterpret_cast<char *>(malloc(CURL_RECV_TTS_VOICE_LEN));
    if (NULL == clock_voice_stru.tts_cache_stru.memory) {
        printf("%s:%d:%s: ERROR[fail malloc callback recv voice buf!] \n", __FILE__, __LINE__, __func__);

        if (clock_voice_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s: ERROR[fail malloc callback recv voice buf!] \n", __func__);
        return -1;
    }

    printf("%s:%d:%s: tts memory init success!\n", __FILE__, __LINE__, __func__);

    if (clock_voice_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s: tts memory init success!\n", __func__);

    return 0;
}

int ClockVoiceProc::memory_deinit() {
    if (clock_voice_stru.tts_cache_stru.memory) {
        free(clock_voice_stru.tts_cache_stru.memory);
        clock_voice_stru.tts_cache_stru.memory = NULL;
    }

    printf("%s:%d:%s: tts memory deinit success!\n", __FILE__, __LINE__, __func__);

    if (clock_voice_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s: tts memory deinit success!\n", __func__);

    return 0;
}

int ClockVoiceProc::time_count_proc() {
    if (clock_voice_stru.clock_setting && clock_voice_stru.time_num > clock_voice_stru.count_time_num) {
        clock_voice_stru.count_time_num++;
    }

    if (clock_voice_stru.clock_setting && clock_voice_stru.time_num == clock_voice_stru.count_time_num) {
        return 1;
    }

    return 0;
}

int ClockVoiceProc::play_select() {
    if (clock_voice_stru.get_clock_voice_status) {
        return 1;
    }

    return 0;
}

int ClockVoiceProc::send_clock_sig() {
    if (clock_voice_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s: clock info tts lock\n",  __func__);
    } else {
        printf("%s:%d:%s: clock info tts lock\n", __FILE__, __LINE__, __func__);
    }

    pthread_mutex_lock(&clock_voice_stru.mutex_cond_stru.txt_mutex);
    clock_voice_stru.tts_voice_action = true;
    pthread_cond_signal(&clock_voice_stru.mutex_cond_stru.voice_cond);
    pthread_mutex_unlock(&clock_voice_stru.mutex_cond_stru.txt_mutex);

    if (clock_voice_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s: clock info tts unlock\n", __func__);
    } else {
        printf("%s:%d:%s: clock info tts unlock\n", __FILE__, __LINE__, __func__);
    }

    return 0;
}

void * ClockVoiceProc::ClockTts(void * arg) {
    printf("%s:%d:%s:###CLOCK VOICE THREAD RUN!###\n", __FILE__, __LINE__, __func__);

    ClockVoiceProc *obj = reinterpret_cast<ClockVoiceProc *>(arg);
    if (clockvoicethreadmod.clock_voice_thread_stop) {
        printf("%s:%d:%s:###tts voice end!###\n", __FILE__, __LINE__, __func__);

        if (obj->clock_voice_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(obj->clock_voice_stru.sendlogobj, DEBUG, "###CLOCK VOICE THREAD END!###");

        pthread_exit(NULL);
    }

    if (obj->clock_voice_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(obj->clock_voice_stru.sendlogobj, DEBUG, "###CLOCK VOICE THREAD RUN!###");

    pthread_mutex_lock(&obj->clock_voice_stru.mutex_cond_stru.txt_mutex);
    while (!clockvoicethreadmod.clock_voice_thread_stop) {
        while (!clockvoicethreadmod.clock_voice_thread_stop && false == obj->clock_voice_stru.tts_voice_action) {
            if (obj->clock_voice_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(obj->clock_voice_stru.sendlogobj, DEBUG, "wait clock voice tts txt!");
            } else {
                DEBUG("wait clock voice tts txt!");
            }

            pthread_cond_wait(&obj->clock_voice_stru.mutex_cond_stru.voice_cond, &obj->clock_voice_stru.mutex_cond_stru.txt_mutex);

            if (obj->clock_voice_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(obj->clock_voice_stru.sendlogobj, DEBUG, "wait clock voice tts txt success!");
            } else {
                DEBUG("wait clock voice tts txt success!");
            }
        }

        obj->clock_voice_stru.tts_voice_action = false;

        if (clockvoicethreadmod.clock_voice_thread_stop) {
            if (obj->clock_voice_stru.client_send_log_status) {
               SENDLOG_CLI_OBJ_PTR(obj->clock_voice_stru.sendlogobj, DEBUG, "%s: clock_voice_thread_stop is true!", __func__);
            } else {
                INFO("%s: clock_voice_thread_stop is true!", __func__);
            }

            break;
        }

        if (0 == obj->get_tts_voice(obj->clock_voice_stru.tts_txt_record_buf)) {
            memset(obj->clock_voice_stru.tts_voice_2_channel_buf, 0, TTS_VOICE_LEN);
            memcpy(obj->clock_voice_stru.tts_voice_2_channel_buf, obj->clock_voice_stru.tts_cache_stru.memory, obj->clock_voice_stru.tts_cache_stru.size);
            obj->clock_voice_stru.clock_voice_channel_2_len = obj->clock_voice_stru.tts_cache_stru.size;
            obj->clock_voice_stru.get_clock_voice_status = true;

            if (obj->clock_voice_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(obj->clock_voice_stru.sendlogobj, DEBUG, "%s: get clock voice sucess!, len:%d", __func__, obj->clock_voice_stru.clock_voice_channel_2_len);
            } else {
                INFO("%s: get clock voice sucess!, len:%d", __func__, obj->clock_voice_stru.clock_voice_channel_2_len);
            }
        } else {
            obj->clock_voice_stru.get_clock_voice_status = false;
            INFO("%s: get clock voice fail!", __func__);
            if (obj->clock_voice_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(obj->clock_voice_stru.sendlogobj, DEBUG, "%s: get clock voice fail!", __func__);
            }
        }
    }
    pthread_mutex_lock(&obj->clock_voice_stru.mutex_cond_stru.txt_mutex);
    pthread_exit(NULL);

    if (obj->clock_voice_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(obj->clock_voice_stru.sendlogobj, DEBUG, "###CLOCK VOICE THREAD END!###\n");
    } else {
        printf("%s:%d:%s:###CLOCK VOICE THREAD END!###\n", __FILE__, __LINE__, __func__);
    }
}

int ClockVoiceProc::set_clock_voice_info_init() {
    if (!clock_voice_stru.clock_setting) {
        clock_voice_stru.count_time_num = 0;
        clock_voice_stru.clock_setting = true;
        return 1;
    }

    return 0;
}

int ClockVoiceProc::set_clock_voice_info_deinit() {
    clock_voice_stru.count_time_num = 0;
    clock_voice_stru.time_num = 0;
    clock_voice_stru.get_clock_voice_status = false;
    clock_voice_stru.clock_setting = false;
    clock_voice_stru.tts_voice_action = false;
    clock_voice_stru.clock_voice_channel_2_len = 0;
    return 0;
}

int ClockVoiceProc::AnalysisAsrResult(const char * asr_result) {
    if (clock_voice_stru.count_time_num == 0) {
        string s = asr_result;
        size_t set_pos = s.find("设置");
        if (0 != set_pos) {
            size_t time_pos = s.find("分钟后");
            if (-1 == time_pos) {
                return -1;
            }

            string time_str = s.substr(0, time_pos);
            string action_str = s.substr(time_pos+9);
            if (0 == action_str.length()) {
                return -1;
            }

            clock_voice_stru.time_num = atoi(time_str.c_str());
            if (0 == clock_voice_stru.time_num) {
                return -1;
            }

            clock_voice_stru.time_num = clock_voice_stru.time_num * 60 *100;
            if (0 == action_str.find("叫我") || 0 == action_str.find("让我")) {
                action_str.replace(0, 6, "");
            }

            if (action_str == string("")) {
                action_str = "主人到时间了";
            } else {
                action_str = "主人到时间了该" + action_str + "了";
            }

            memset(clock_voice_stru.tts_txt_record_buf, 0, sizeof(clock_voice_stru.tts_txt_record_buf));
            memcpy(clock_voice_stru.tts_txt_record_buf, action_str.c_str(), action_str.length());
        } else {
            size_t time_pos = s.find("分钟后");
            if (-1 == time_pos) {
                return -1;
            }

            string time_str = s.substr(set_pos + 6, time_pos-set_pos-6);
            string action_str = s.substr(time_pos+9);
            if (0 == action_str.length()) {
                return -1;
            }

            clock_voice_stru.time_num = atoi(time_str.c_str());
            if (0 == clock_voice_stru.time_num) {
                return -1;
            }

            clock_voice_stru.time_num = clock_voice_stru.time_num * 60 *100;
            if (0 == action_str.find("叫我") || 0 == action_str.find("让我")) {
                action_str.replace(0, 6, "");
            }

            if (action_str == string("")) {
                action_str = "主人到时间了";
            } else {
                action_str = "主人到时间了该" + action_str + "了";
            }

            cout << action_str << endl;

            memset(clock_voice_stru.tts_txt_record_buf, 0, sizeof(clock_voice_stru.tts_txt_record_buf));
            memcpy(clock_voice_stru.tts_txt_record_buf, action_str.c_str(), action_str.length());
        }
    } else {
        return -1;
    }

    return 0;
}

int ClockVoiceProc::write_ttsdata(void * buffer, int size, int nmemb, void *userp) {
    TTS_CALLBACK_MEMORY * mem = reinterpret_cast<TTS_CALLBACK_MEMORY*>(userp);
    memcpy(&mem->memory[mem->size], buffer, size * nmemb);
    mem->size += size * nmemb;

    return size * nmemb;
}

int ClockVoiceProc::get_tts_voice(char *tts_curldata) {
    if (clock_voice_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "into get tts voice!");
    } else {
        printf("%s:%d:%s into get tts voice!\n", __FILE__, __LINE__, __func__);
    }

    int64_t ret_code = 0;
    char param_data[1024];
    clock_voice_stru.curl_stru._headers = NULL;

    snprintf(param_data, sizeof(param_data), "text=%s&user=10123565085&speed=5&volume=5&pitch=5&audioType=4", tts_curldata);
    std::string format_txt = reinterpret_cast<char *>(param_data);
    std::string Tts_Param = encode_to_url(format_txt);

    clock_voice_stru.curl_stru.tts_curl = curl_easy_init();
    clock_voice_stru.curl_stru._headers = curl_slist_append(clock_voice_stru.curl_stru._headers, "Accept-Encoding: gzip");
    clock_voice_stru.curl_stru._headers = curl_slist_append(clock_voice_stru.curl_stru._headers, "channel: cloudasr");
    clock_voice_stru.curl_stru._headers = curl_slist_append(clock_voice_stru.curl_stru._headers, LENOVOKEY_FRAMEWORK);
    clock_voice_stru.curl_stru._headers = curl_slist_append(clock_voice_stru.curl_stru._headers, SECRETKEY_FRAMEWORK);

    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_HTTPHEADER, clock_voice_stru.curl_stru._headers);
    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_POSTFIELDS, Tts_Param.c_str());
    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_URL, TTS_POSTURL);
    // curl_easy_setopt(tts_curl, CURLOPT_URL, TTS_GETURL);
    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // curl_easy_setopt(tts_curl, CURLOPT_CONNECTTIMEOUT, 1L);
    // curl_easy_setopt(tts_curl, CURLOPT_TIMEOUT, 120);
    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_NOSIGNAL, 1L);
    // curl_easy_setopt(tts_curl, CURLOPT_NOPROGRESS, 1L);

    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_LOW_SPEED_TIME, 15L);
    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_CONNECTTIMEOUT, 15L);

    clock_voice_stru.tts_cache_stru.size = 0;
    memset(clock_voice_stru.tts_cache_stru.memory, 0, CURL_RECV_TTS_VOICE_LEN);

    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_WRITEDATA, reinterpret_cast<void *>(&clock_voice_stru.tts_cache_stru));
    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_WRITEFUNCTION, write_ttsdata);
    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_HEADER, 0L);
    curl_easy_setopt(clock_voice_stru.curl_stru.tts_curl, CURLOPT_VERBOSE, 0L);

    CURLcode ret_curl = curl_easy_perform(clock_voice_stru.curl_stru.tts_curl);
    if (ret_curl != CURLE_OK) {
        printf("%s:%d:%s: ERROR[%s]\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl));

        if (clock_voice_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, " ERROR[%s]\n", curl_easy_strerror(ret_curl));

        curl_slist_free_all(clock_voice_stru.curl_stru._headers);
        curl_easy_cleanup(clock_voice_stru.curl_stru.tts_curl);
        return -1;
    }

    ret_curl = curl_easy_getinfo(clock_voice_stru.curl_stru.tts_curl, CURLINFO_RESPONSE_CODE, &ret_code);
    if ((CURLE_OK == ret_curl) && ret_code == 200) {
        DEBUG("%s: url recv tts voice size :%d", __func__, clock_voice_stru.tts_cache_stru.size);

        if (clock_voice_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s: url recv tts voice size :%d", __func__, clock_voice_stru.tts_cache_stru.size);
    } else {
        printf("%s:%d:%s: ERROR[%s] ret_code:%d\n", __FILE__, __LINE__, __func__, curl_easy_strerror(ret_curl), ret_code);

        if (clock_voice_stru.client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, " ERROR[%s] ret_code:%d\n", curl_easy_strerror(ret_curl), ret_code);

        curl_slist_free_all(clock_voice_stru.curl_stru._headers);
        curl_easy_cleanup(clock_voice_stru.curl_stru.tts_curl);
        return -1;
    }

    curl_slist_free_all(clock_voice_stru.curl_stru._headers);
    curl_easy_cleanup(clock_voice_stru.curl_stru.tts_curl);
    printf("%s:%d:%s get clock tts voice end! tts len:%d\n", __FILE__, __LINE__, __func__, clock_voice_stru.tts_cache_stru.size);

    if (clock_voice_stru.client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(clock_voice_stru.sendlogobj, DEBUG, "%s get clock tts voice end! tts len:%d\n", __func__, clock_voice_stru.tts_cache_stru.size);

    return 0;
}

std::string ClockVoiceProc::encode_to_url(std::string &str_source) {
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
