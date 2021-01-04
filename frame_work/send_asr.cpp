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
#include "send_asr.h"

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

ASRTHREAD_MODULE_INFO asrthreadmod;

InteractiveProcForASR * InteractiveProcForASR::static_obj = NULL;
volatile bool InteractiveProcForASR::client_send_log_status = false;

int InteractiveProcForASR::Init() {
#if __SEND_LOG_TO_SERVER__
    asr_proc_info.sendlogobj = new SendLogProc();
    if (NULL == asr_proc_info.sendlogobj) {
        printf("%s:%d:%s: ERROR [thread malloc err!]\n", __FILE__, __LINE__, __func__);
        client_send_log_status = false;
    } else {
        client_send_log_status = true;
    }

    if (-1 == asr_proc_info.sendlogobj->Init()) {
        printf("%s:%d:%s: ERROR [send log Init err!]\n", __FILE__, __LINE__, __func__);
        client_send_log_status = false;
    } else {
        client_send_log_status = true;
    }
#endif

    InitSendAsrCurlData();
    mutex_cond_init();
    if (-1 == sendbuf_init()) {
        return -1;
    }

    if (-1 == cache_pcm_buf_init()) {
        return -1;
    }

    if (!start_thread()) {
        return -1;
    }

    printf("%s:%s:asr proc init success!\n", __FILE__, __func__);

    if (client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:asr proc init success!\n", __func__);

    return 0;
}


int InteractiveProcForASR::DeInit() {
    DEBUG("INTO ASR PROC DEINIT");

    if (client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "INTO ASR PROC DEINIT");

    pthread_mutex_lock(&asr_mutex_cond.asr_send_mutex);
    pthread_cond_broadcast(&asr_mutex_cond.asr_cond_process);
    pthread_mutex_unlock(&asr_mutex_cond.asr_send_mutex);

    delete_thread();
    sendbuf_deinit();
    cache_pcm_buf_deinit();
    mutex_cond_deinit();
    DeInitSendAsrCurlData();

#if __SEND_LOG_TO_SERVER__
    if (NULL != asr_proc_info.sendlogobj) {
        asr_proc_info.sendlogobj->DeInit();
        delete asr_proc_info.sendlogobj;
    }
#endif

    printf("%s:%d:%s:asr proc deinit success!\n", __FILE__, __LINE__, __func__);

    return 0;
}

void InteractiveProcForASR::send_asr_buf_queue_push(SEND_500MS_QUEUE **head, SEND_500MS_QUEUE *node) {
    SEND_500MS_QUEUE * iter;

    node->next = NULL;

    if (NULL == (*head)) {
        *head = node;
    } else {
        iter = *head;
        while (iter->next) {
            iter = iter->next;
        }
        iter->next = node;
    }
}

SEND_500MS_QUEUE * InteractiveProcForASR::send_asr_buf_queue_pop(SEND_500MS_QUEUE **head) {
    SEND_500MS_QUEUE * node = (*head);
    if (NULL != node) {
        *head = node->next;
        node->next = NULL;
    }
    return node;
}

int InteractiveProcForASR::sendbuf_init() {
    int packet_len = CACHE_BUF_NUM * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS + 4;

    asr_proc_info.send_500ms_queue_total_buf = reinterpret_cast<void *>(calloc(packet_len, SEND_500MS_BUF_NUM));
    if (NULL == asr_proc_info.send_500ms_queue_total_buf) {
        printf("%s:%s:fail allocate 500ms queue total buf! \n", __FILE__, __func__);
        if (client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:fail allocate 500ms queue total buf! \n", __func__);
        return -1;
    }

    for (int i = 0 ; i < SEND_500MS_BUF_NUM ; ++i) {
        SEND_500MS_QUEUE *buf = &asr_proc_info.send_asr_nodes[i];
        buf->data = &((reinterpret_cast<char *>(asr_proc_info.send_500ms_queue_total_buf))[i * packet_len ]);
        buf->data[0] = 5;
        buf->data[1] = 0;
        buf->data[2] = 0;
        buf->data[3] = 0;
        buf->vadflag = true;
        send_asr_buf_queue_push(&asr_proc_info.send_asr_buf_free, buf);
    }

    return 0;
}

int InteractiveProcForASR::cache_pcm_buf_init() {
    const int buf_size = CACHE_BUF_NUM * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS;
    asr_proc_info.cache_10ms_stru.total_pingpong_buf = reinterpret_cast<void *>(calloc(buf_size, 2));
    if (NULL == asr_proc_info.cache_10ms_stru.total_pingpong_buf) {
        printf("%s:%d:%s: ERROR [malloc total_pingpong_buf fail!]\n", __FILE__, __LINE__, __func__);
        if (client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: ERROR [malloc total_pingpong_buf fail!]\n", __func__);
        return -1;
    }

    for (int i = 0; i < 2; ++i) {
        asr_proc_info.cache_10ms_stru.copy_pingpong_buf[i] = &((reinterpret_cast<char *>(asr_proc_info.cache_10ms_stru.total_pingpong_buf))[i * buf_size]);
    }

    return 0;
}

int InteractiveProcForASR::sendbuf_deinit() {
    if (asr_proc_info.send_500ms_queue_total_buf) {
        free(asr_proc_info.send_500ms_queue_total_buf);
        asr_proc_info.send_500ms_queue_total_buf = NULL;
    }

    asr_proc_info.send_asr_buf_free = NULL;
    asr_proc_info.send_asr_buf = NULL;

    printf("%s:%d:%s: asr send 500ms queue deinit!\n", __FILE__, __LINE__, __func__);

    if (client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr send 500ms queue deinit!\n",  __func__);

    return 0;
}

int InteractiveProcForASR::cache_pcm_buf_deinit() {
    if (asr_proc_info.cache_10ms_stru.total_pingpong_buf) {
        free(asr_proc_info.cache_10ms_stru.total_pingpong_buf);
        asr_proc_info.cache_10ms_stru.total_pingpong_buf = NULL;
    }

    asr_proc_info.cache_10ms_stru.copy_pingpong_buf[0] = nullptr;
    asr_proc_info.cache_10ms_stru.copy_pingpong_buf[1] = nullptr;

    printf("%s:%d:%s: cache 10ms queue deinit!\n", __FILE__, __LINE__, __func__);

    if (client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: cache 10ms queue deinit!\n", __func__);

    return 0;
}

int InteractiveProcForASR::mutex_cond_init() {
    pthread_mutex_init(&asr_mutex_cond.asr_send_mutex, NULL);
    pthread_cond_init(&asr_mutex_cond.asr_cond_process, NULL);

    return 0;
}

int InteractiveProcForASR::mutex_cond_deinit() {
    pthread_mutex_destroy(&asr_mutex_cond.asr_send_mutex);
    pthread_cond_destroy(&asr_mutex_cond.asr_cond_process);
    printf("%s:%d:%s: mutex and cond deinit!\n", __FILE__, __LINE__, __func__);

    if (client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: mutex and cond deinit!\n", __func__);

    return 0;
}

bool InteractiveProcForASR::start_thread() {
    int ret = 0;
    ret = pthread_create(&asr_proc_info._thr_process_id, NULL, thread_process, this);
    if (0 != ret) {
        printf("%s:%s:thread create error!", __FILE__, __func__);
        if (client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:thread create error!", __func__);
        return false;
    }

    return true;
}

int InteractiveProcForASR::delete_thread() {
    DEBUG("INTO ASR THREAD DEINIT");

    if (client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "INTO ASR THREAD DEINIT");
    pthread_join(asr_proc_info._thr_process_id, NULL);
    printf("%s:%d:%s:asr thread deinit success!\n", __FILE__, __LINE__, __func__);

    if (client_send_log_status)
        SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:asr thread deinit success!\n", __func__);

    return 0;
}

int InteractiveProcForASR::InitSendAsrCurlData() {
    asr_proc_info.curl_start_session = true;

    asr_proc_info.recv_stru.memory = asr_proc_info.curl_asr_stru.asr_response_buf;
    asr_proc_info.recv_stru.size = ASR_RESPONSE_MAX_MSG + 1;

    asr_proc_info.curl_asr_stru._header = NULL;
    asr_proc_info.curl_asr_stru._list_par = NULL;
    asr_proc_info.curl_asr_stru._list_voi = NULL;

    asr_proc_info.curl_asr_stru._header = curl_slist_append(asr_proc_info.curl_asr_stru._header, "Accept-Encoding: gzip");
    asr_proc_info.curl_asr_stru._header = curl_slist_append(asr_proc_info.curl_asr_stru._header, "Expect:");

    asr_proc_info.curl_asr_stru._header = curl_slist_append(asr_proc_info.curl_asr_stru._header, "channel: cloudasr");
    asr_proc_info.curl_asr_stru._header = curl_slist_append(asr_proc_info.curl_asr_stru._header, LENOVOKEY_FRAMEWORK);
    asr_proc_info.curl_asr_stru._header = curl_slist_append(asr_proc_info.curl_asr_stru._header, SECRETKEY_FRAMEWORK);

    asr_proc_info.curl_asr_stru._list_par = curl_slist_append(asr_proc_info.curl_asr_stru._list_par, "Content-Transfer-Encoding: 8bit");
    asr_proc_info.curl_asr_stru._list_voi = curl_slist_append(asr_proc_info.curl_asr_stru._list_voi, "Content-Transfer-Encoding: binary");

    return 0;
}

int InteractiveProcForASR::DeInitSendAsrCurlData() {
    curl_slist_free_all(asr_proc_info.curl_asr_stru._header);
    curl_slist_free_all(asr_proc_info.curl_asr_stru._list_par);
    curl_slist_free_all(asr_proc_info.curl_asr_stru._list_voi);

    if (client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: curl_slist_free success!\n", __func__);
    } else {
        printf("%s:%d:%s: curl_slist_free success!\n", __FILE__, __LINE__, __func__);
    }

    return 0;
}

int InteractiveProcForASR::asr_write_data(void *buffer, int size, int nmemb, void **stream) {
    if (client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(static_obj->asr_proc_info.sendlogobj, DEBUG, "%s:size *nmemb :%d\n", __func__, size *nmemb);
    } else {
        INFO("%s:size *nmemb :%d\n", __func__, size * nmemb);
    }

    memset(static_obj->asr_proc_info.recv_stru.memory, 0, ASR_RESPONSE_MAX_MSG + 1);
    memcpy(static_obj->asr_proc_info.recv_stru.memory, buffer, (size_t)(size*nmemb));

    return size * nmemb;
}

int InteractiveProcForASR::init_curl_session() {
    asr_proc_info.curl_start_session = false;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long _stmm = (long long)tv.tv_sec*1000 + tv.tv_usec/1000;
    memset(asr_proc_info.curl_asr_stru._curl_stime, 0, sizeof(asr_proc_info.curl_asr_stru._curl_stime));
    snprintf(asr_proc_info.curl_asr_stru._curl_stime, sizeof(asr_proc_info.curl_asr_stru._curl_stime), "%lld", _stmm);

    asr_proc_info.curl_asr_stru._formpost = NULL;
    asr_proc_info.curl_asr_stru._last_ptr = NULL;
    asr_proc_info.curl_asr_stru._pCurl = NULL;
    asr_proc_info.curl_asr_stru._pCurl = curl_easy_init();

    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_HTTPHEADER, asr_proc_info.curl_asr_stru._header);
    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_URL, POSTASRURL);
    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_POST, 1L);

    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_WRITEFUNCTION, asr_write_data);

    // curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl,CURLOPT_HTTPHEADER,"Expect:");
    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_HEADER, 0L);
    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_VERBOSE, 0L);

    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_LOW_SPEED_TIME, 15L);
    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_CONNECTTIMEOUT, 15L);

    if (client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:开始curl会话", __func__);
    } else {
        DEBUG("%s:开始curl会话 ixid:%s", __func__, asr_proc_info.curl_asr_stru._curl_stime);
    }

    return 0;
}

int InteractiveProcForASR::deinit_curl_session() {
    if (false == asr_proc_info.curl_start_session) {
        curl_easy_cleanup(asr_proc_info.curl_asr_stru._pCurl);
        asr_proc_info.curl_start_session = true;

        if (client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:释放curl会话句柄！", __func__);
        } else {
            DEBUG("%s:释放curl会话句柄！", __func__);
        }
    }

    return 0;
}

int InteractiveProcForASR::SendDataToAsr(char *send_buf, int send_buf_len, int packets_num, bool vadflag) {
    char param_data[1024] = {0};
    asr_proc_info.curl_asr_stru._formpost = NULL;
    asr_proc_info.curl_asr_stru._last_ptr = NULL;

    if (!asr_proc_info.start_new_session&& 0 != asr_proc_info.send_pakets_count) {
        if (vadflag) {
            snprintf(param_data, sizeof(param_data),
                    "app=com.lenovo.menu_assistant&over=1&dtp=motorola_ali&ver=1.0.9&cpts=0&city=北京市&spts=0&ssm=true&stm=%s&rvr=&ntt=wifi&aue=speex-wb;7&uid=37136136&auf=audio/L16;rate=16000&dev=lenovo.ecs.vt&sce=cmd&rsts=0&lrts=0&pidx=%ld&fpts=0&ixid=%s&vdm=all&did=270931c188c281ca&key=27802c5aa9475eacbeff13d04e4da0a8&n2lang=multi&",
                    asr_proc_info.curl_asr_stru._curl_stime, packets_num, asr_proc_info.curl_asr_stru._curl_stime);
            // snprintf(param_data, sizeof(param_data),
            //        "app=com.lenovo.menu_assistant&over=1&dtp=motorola_ali&ver=1.0.9&cpts=0&city=北京市&spts=0&ssm=true&stm=%s&rvr=&ntt=wifi&aue=speex-wb;7&uid=37136136&auf=audio/L16;rate=16000&dev=lenovo.ecs.vt&sce=cmd&rsts=0&lrts=0&pidx=%ld&fpts=0&ixid=%s&vdm=all&did=270931c188c281ca&key=27802c5aa9475eacbeff13d04e4da0a8&",
            //        asr_proc_info.curl_asr_stru._curl_stime, packets_num, asr_proc_info.curl_asr_stru._curl_stime);
        } else {
            snprintf(param_data, sizeof(param_data),
                    "app=com.lenovo.menu_assistant&over=0&dtp=motorola_ali&ver=1.0.9&cpts=0&city=北京市&spts=0&ssm=true&stm=%s&rvr=&ntt=wifi&aue=speex-wb;7&uid=37136136&auf=audio/L16;rate=16000&dev=lenovo.ecs.vt&sce=cmd&rsts=0&lrts=0&pidx=%ld&fpts=0&ixid=%s&vdm=all&did=270931c188c281ca&key=27802c5aa9475eacbeff13d04e4da0a8&n2lang=multi&",
                    asr_proc_info.curl_asr_stru._curl_stime, packets_num, asr_proc_info.curl_asr_stru._curl_stime);
            // snprintf(param_data, sizeof(param_data),
            //        "app=com.lenovo.menu_assistant&over=0&dtp=motorola_ali&ver=1.0.9&cpts=0&city=北京市&spts=0&ssm=true&stm=%s&rvr=&ntt=wifi&aue=speex-wb;7&uid=37136136&auf=audio/L16;rate=16000&dev=lenovo.ecs.vt&sce=cmd&rsts=0&lrts=0&pidx=%ld&fpts=0&ixid=%s&vdm=all&did=270931c188c281ca&key=27802c5aa9475eacbeff13d04e4da0a8&",
            //        asr_proc_info.curl_asr_stru._curl_stime, packets_num, asr_proc_info.curl_asr_stru._curl_stime);
        }
    } else {
        return -1;
    }

    if (!asr_proc_info.start_new_session && 0 != asr_proc_info.send_pakets_count) {
        curl_formadd(&asr_proc_info.curl_asr_stru._formpost, &asr_proc_info.curl_asr_stru._last_ptr, CURLFORM_COPYNAME,
            "param-data", CURLFORM_COPYCONTENTS, param_data, CURLFORM_CONTENTHEADER, asr_proc_info.curl_asr_stru._list_par,
            CURLFORM_CONTENTTYPE, "text/plain; charset=UTF-8", CURLFORM_END);
    } else {
        return -1;
    }

    if (!asr_proc_info.start_new_session && 0 != asr_proc_info.send_pakets_count) {
        curl_formadd(&asr_proc_info.curl_asr_stru._formpost, &asr_proc_info.curl_asr_stru._last_ptr, CURLFORM_COPYNAME,
            "voice-data", CURLFORM_BUFFER, "voice.dat", CURLFORM_BUFFERPTR, send_buf, CURLFORM_BUFFERLENGTH, send_buf_len,
            CURLFORM_CONTENTHEADER, asr_proc_info.curl_asr_stru._list_voi, CURLFORM_CONTENTTYPE, "application/i7-voice", CURLFORM_END);
    } else {
        return -1;
    }

    if (!asr_proc_info.start_new_session && 0 != asr_proc_info.send_pakets_count) {
        curl_easy_setopt(asr_proc_info.curl_asr_stru._pCurl, CURLOPT_HTTPPOST, asr_proc_info.curl_asr_stru._formpost);
    } else {
        curl_formfree(asr_proc_info.curl_asr_stru._formpost);
        return -1;
    }

    if (!asr_proc_info.start_new_session && 0 != asr_proc_info.send_pakets_count) {
        if (client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:curl_easy_perform开始执行，vad:%d，pakets_num:%d", __func__, vadflag, packets_num);
        } else {
            DEBUG("%s:curl_easy_perform开始执行，vad:%d，pakets_num:%d", __func__, vadflag, packets_num);
        }

        CURLcode ret_curl = curl_easy_perform(asr_proc_info.curl_asr_stru._pCurl);
        if (CURLE_OK != ret_curl) {
            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:CURL 请求 失败:  CURLcode=%d errinfo:%s\n", __func__, ret_curl, curl_easy_strerror(ret_curl));
            } else {
                printf("%s:%s:CURL 请求 失败: CURLcode=%d errinfo:%s\n", __FILE__, __func__, ret_curl, curl_easy_strerror(ret_curl));
            }
            curl_formfree(asr_proc_info.curl_asr_stru._formpost);

            if (ASR_DEFAULT ==  asrthreadmod.old_asr_mod && 0 != asr_proc_info.send_pakets_count) {
                asrthreadmod.old_asr_mod = ASR_NET_ERR;
                asrthreadmod.asr_mod = ASR_NET_ERR;
                memset(asr_proc_info.recv_stru.memory, 0, asr_proc_info.recv_stru.size);
                asrthreadmod.global_asr_res = true;
                if (client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:播放网络问题，提示音！", __func__);
                } else {
                    DEBUG("%s:播放网络问题，提示音！", __func__);
                }
            }

            if (!asr_proc_info.start_new_session && 0 != asr_proc_info.send_pakets_count) {
                asr_proc_info.global_awake_voice_send = false;
                asr_proc_info.start_new_session = true;

                if (client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:发送asr服务器线程暂停，同时500msbuf也不会入队列", __func__);
                } else {
                    DEBUG("%s:发送asr服务器线程暂停，同时500msbuf也不会入队列", __func__);
                }
            }

            return -1;
        } else {
            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:curl_easy_perform执行成功，vad:%d，pakets_num:%d", __func__, vadflag, packets_num);
            } else {
                DEBUG("%s:curl_easy_perform执行成功，vad:%d，pakets_num:%d", __func__, vadflag, packets_num);
            }

            if (!asr_proc_info.start_new_session && 0 != asr_proc_info.send_pakets_count) {
                if (vadflag) {
                    long ret_code = 0;
                    if (!asr_proc_info.start_new_session && 0 != asr_proc_info.send_pakets_count) {
                        if (client_send_log_status) {
                            SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:curl_easy_getinfo开始执行，vad:%d，pakets_num:%d", __func__, vadflag, packets_num);
                        } else {
                            DEBUG("%s:curl_easy_getinfo开始执行 vad:%d，pakets_num:%d", __func__, vadflag, packets_num);
                        }

                        ret_curl = curl_easy_getinfo(asr_proc_info.curl_asr_stru._pCurl, CURLINFO_RESPONSE_CODE, &ret_code);

                        if (client_send_log_status) {
                            SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:curl_easy_getinfo执行完成， vad:%d，pakets_num:%d", __func__, vadflag, packets_num);
                        } else {
                            DEBUG("%s:curl_easy_getinfo执行完成，vad:%d，pakets_num:%d", __func__, vadflag, packets_num);
                        }

                        if ((CURLE_OK == ret_curl) && ret_code == 200) {
                            if (client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:asr server response msg:%s", __func__, asr_proc_info.recv_stru.memory);
                            } else {
                                INFO("%s:asr server response msg:%s", __func__, asr_proc_info.recv_stru.memory);
                            }

                            curl_formfree(asr_proc_info.curl_asr_stru._formpost);

                            if (ASR_DEFAULT ==  asrthreadmod.old_asr_mod && 0 != asr_proc_info.send_pakets_count) {
                                asrthreadmod.old_asr_mod = ASR_NO_HEAR;
                                asrthreadmod.global_asr_res = true;
                                if (client_send_log_status) {
                                    SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:提示音！", __func__);
                                } else {
                                    DEBUG("%s:提示音！", __func__);
                                }
                            }

                            return 0;
                        } else {
                            if (client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:%s:获取asr响应码失败! ret_curl:%d ret_code:%d errinfo:%s", __FILE__,
                                    __func__, ret_curl, ret_code, curl_easy_strerror(ret_curl));
                            } else {
                                printf("%s:%s:获取asr响应码失败! ret_curl:%d ret_code:%d errinfo:%s",
                                        __FILE__, __func__, ret_curl, ret_code, curl_easy_strerror(ret_curl));
                            }

                            curl_formfree(asr_proc_info.curl_asr_stru._formpost);

                            if (ASR_DEFAULT ==  asrthreadmod.old_asr_mod && 0 != asr_proc_info.send_pakets_count) {
                                asrthreadmod.old_asr_mod = ASR_NO_HEAR;
                                memset(asr_proc_info.recv_stru.memory, 0, asr_proc_info.recv_stru.size);
                                asrthreadmod.global_asr_res = true;

                                if (client_send_log_status) {
                                    SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:提示音！", __func__);
                                } else {
                                    DEBUG("%s:提示音！", __func__);
                                }
                            }

                            return -1;
                        }
                    }
                }
            }
        }
    }

    curl_formfree(asr_proc_info.curl_asr_stru._formpost);

    if (asr_proc_info.start_new_session) {
        return -1;
    }

    return 0;
}

int InteractiveProcForASR::cache_send_buf(char *cache_buf_tmp, char *read_buf, int &cache_num) {
    memcpy(cache_buf_tmp + cache_num * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS, read_buf, SAMPLENUM * SAMPLEBYTES * IN_CHANNELS);
    return ++cache_num;
}

int InteractiveProcForASR::GetAsrResult(char *in) {
    memset(in, 0, ASR_RESPONSE_MAX_MSG +1);
    memcpy(in, asr_proc_info.recv_stru.memory, asr_proc_info.recv_stru.size);
    return 0;
}

int InteractiveProcForASR::pause() {
    asr_proc_info.global_awake_voice_send = true;
    asr_proc_info.start_new_session = true;
    asr_proc_info.awake_random_pingpong = true;
    asrthreadmod.global_asr_res = false;
    return 0;
}

int InteractiveProcForASR::set_send_asr_info(char *send_buf, int send_buf_len, bool vadfg) {
    if (asr_proc_info.global_awake_voice_send) {
        if (asr_proc_info.start_new_session) {
            int tmp_count = 0;

            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr in pcm queue lock\n", __func__);
            } else {
                printf("%s:%d:%s: asr in pcm queue lock\n", __FILE__, __LINE__, __func__);
            }

            pthread_mutex_lock(&asr_mutex_cond.asr_send_mutex);
            while (NULL != asr_proc_info.send_asr_buf) {
                SEND_500MS_QUEUE *new_session_free_buf = send_asr_buf_queue_pop(&asr_proc_info.send_asr_buf);
                new_session_free_buf->vadflag = true;
                send_asr_buf_queue_push(&asr_proc_info.send_asr_buf_free, new_session_free_buf);
                ++tmp_count;
            }
            pthread_mutex_unlock(&asr_mutex_cond.asr_send_mutex);

            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr in pcm queue unlock\n", __func__);
            } else {
                printf("%s:%d:%s: asr in pcm queue unlock\n", __FILE__, __LINE__, __func__);
            }

            asr_proc_info.start_new_session = false;
            asr_proc_info.send_pakets_count = 0;
            asrthreadmod.old_asr_mod = ASR_DEFAULT;

            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:开始新任务，已清空500ms队列,清空次数%d！", __func__, tmp_count);
            } else {
                DEBUG("%s:开始新任务，已清空500ms队列,清空次数%d！", __func__, tmp_count);
            }
        }

        if (NULL == asr_proc_info.send_asr_buf_free && false == asr_proc_info.cache_10ms_stru.already_get_500ms_buf) {
            asr_proc_info.send_pakets_count = 0;
            asr_proc_info.start_new_session = true;
            asr_proc_info.global_awake_voice_send = false;
            asr_proc_info.awake_random_pingpong = true;

            int tmp_count = 0;

            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr in pcm queue lock\n", __func__);
            } else {
                printf("%s:%d:%s: asr in pcm queue lock\n", __FILE__, __LINE__, __func__);
            }

            pthread_mutex_lock(&asr_mutex_cond.asr_send_mutex);
            while (NULL != asr_proc_info.send_asr_buf) {
                SEND_500MS_QUEUE *new_session_free_buf = send_asr_buf_queue_pop(&asr_proc_info.send_asr_buf);
                new_session_free_buf->vadflag = true;
                send_asr_buf_queue_push(&asr_proc_info.send_asr_buf_free, new_session_free_buf);
                ++tmp_count;
            }
            pthread_mutex_unlock(&asr_mutex_cond.asr_send_mutex);

            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr in pcm queue unlock\n", __func__);
            } else {
                printf("%s:%d:%s: asr in pcm queue unlock\n", __FILE__, __LINE__, __func__);
            }

            if (ASR_DEFAULT == asrthreadmod.old_asr_mod) {
                asrthreadmod.asr_mod = ASR_NET_ERR;
                asrthreadmod.global_asr_res = true;

                if (client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:提示网络问题，提示音！", __func__);
                } else {
                    DEBUG("%s:提示网络问题，提示音！", __func__);
                }
            }

            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:send_asr_buf_free仍为NULL,清空队列计数：%d,结束当前会话！", __func__, tmp_count);
            } else {
                DEBUG("%s:send_asr_buf_free仍为NULL,清空队列计数：%d,结束当前会话！", __func__, tmp_count);
            }

            goto queue_proc;
        }

        if (NULL != asr_proc_info.send_asr_buf_free && false == asr_proc_info.cache_10ms_stru.already_get_500ms_buf) {
            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr in pcm queue lock\n", __func__);
            } else {
                printf("%s:%d:%s: asr in pcm queue lock\n", __FILE__, __LINE__, __func__);
            }

            pthread_mutex_lock(&asr_mutex_cond.asr_send_mutex);
            asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr = send_asr_buf_queue_pop(&asr_proc_info.send_asr_buf_free);
            pthread_mutex_unlock(&asr_mutex_cond.asr_send_mutex);

            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr in pcm queue unlock\n", __func__);
            } else {
                printf("%s:%d:%s: asr in pcm queue unlock\n", __FILE__, __LINE__, __func__);
            }

            asr_proc_info.cache_10ms_stru.already_get_500ms_buf = true;

            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:get 500msbuf for write", __func__);
            } else {
                DEBUG("%s:get 500msbuf for write", __func__);
            }
        }

        if (asr_proc_info.awake_random_pingpong) {
            asr_proc_info.awake_random_pingpong = false;
            if (asr_proc_info.cache_10ms_stru.cache_num < (CACHE_BUF_NUM - 1)) {
                memset(asr_proc_info.cache_10ms_stru.copy_pingpong_buf[asr_proc_info.cache_10ms_stru.copy_id] + asr_proc_info.cache_10ms_stru.cache_num * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS,
                    0, (CACHE_BUF_NUM - asr_proc_info.cache_10ms_stru.cache_num) * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS);
            }

            asr_proc_info.cache_10ms_stru.cache_num = 0;
            asr_proc_info.cache_10ms_stru.cache_total_count = 0;

            asr_proc_info.cache_10ms_stru.already_get_500ms_buf = false;
            memcpy((asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr->data) + 4,
                asr_proc_info.cache_10ms_stru.copy_pingpong_buf[asr_proc_info.cache_10ms_stru.copy_id],
                CACHE_BUF_NUM * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS);
            asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr->vadflag = true;

            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr in pcm queue lock\n", __func__);
            } else {
                printf("%s:%d:%s: asr in pcm queue lock\n", __FILE__, __LINE__, __func__);
            }

            pthread_mutex_lock(&asr_mutex_cond.asr_send_mutex);
            send_asr_buf_queue_push(&asr_proc_info.send_asr_buf, asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr);
            pthread_mutex_unlock(&asr_mutex_cond.asr_send_mutex);

            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr in pcm queue unlock\n", __func__);
            } else {
                printf("%s:%d:%s: asr in pcm queue unlock\n", __FILE__, __LINE__, __func__);
            }

            asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr = NULL;

            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:发送信号，取500ms队列,随机buf", __func__);
            } else {
                DEBUG("%s:发送信号，取500ms队列,随机buf", __func__);
            }

            pthread_cond_signal(&asr_mutex_cond.asr_cond_process);
        } else {
            if ((false == vadfg && asr_proc_info.cache_10ms_stru.cache_total_count > AWAKE_WAIT_TIME_NUM)
                    || asr_proc_info.cache_10ms_stru.cache_total_count > AWAKE_WAIT_TIME_NUM_MAX) {
                asr_proc_info.global_awake_voice_send = false;
                asr_proc_info.awake_random_pingpong = true;

                cache_send_buf(asr_proc_info.cache_10ms_stru.copy_pingpong_buf[asr_proc_info.cache_10ms_stru.copy_id], send_buf, asr_proc_info.cache_10ms_stru.cache_num);

                if (asr_proc_info.cache_10ms_stru.cache_num < (CACHE_BUF_NUM - 1)) {
                    memset(asr_proc_info.cache_10ms_stru.copy_pingpong_buf[asr_proc_info.cache_10ms_stru.copy_id] + asr_proc_info.cache_10ms_stru.cache_num * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS,
                        0, (CACHE_BUF_NUM - asr_proc_info.cache_10ms_stru.cache_num) * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS);
                }

                asr_proc_info.cache_10ms_stru.cache_num = 0;
                asr_proc_info.cache_10ms_stru.cache_total_count = 0;

                asr_proc_info.cache_10ms_stru.already_get_500ms_buf = false;
                memcpy((asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr->data) + 4,
                    asr_proc_info.cache_10ms_stru.copy_pingpong_buf[asr_proc_info.cache_10ms_stru.copy_id],
                    CACHE_BUF_NUM * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS);
                asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr->vadflag = false;

                if (client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr in pcm queue lock\n", __func__);
                } else {
                    printf("%s:%d:%s: asr in pcm queue lock\n", __FILE__, __LINE__, __func__);
                }

                pthread_mutex_lock(&asr_mutex_cond.asr_send_mutex);
                send_asr_buf_queue_push(&asr_proc_info.send_asr_buf, asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr);
                pthread_mutex_unlock(&asr_mutex_cond.asr_send_mutex);

                if (client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr in pcm queue unlock\n", __func__);
                } else {
                    printf("%s:%d:%s: asr in pcm queue unlock\n", __FILE__, __LINE__, __func__);
                }

                asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr = NULL;

                if (client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:发送信号，取500ms队列,尾包", __func__);
                } else {
                    DEBUG("%s:发送信号，取500ms队列,尾包", __func__);
                }

                pthread_cond_signal(&asr_mutex_cond.asr_cond_process);
            } else {
                ++asr_proc_info.cache_10ms_stru.cache_total_count;
                int ret = cache_send_buf(asr_proc_info.cache_10ms_stru.copy_pingpong_buf[asr_proc_info.cache_10ms_stru.copy_id], send_buf, asr_proc_info.cache_10ms_stru.cache_num);
                if (CACHE_BUF_NUM == ret) {
                    asr_proc_info.cache_10ms_stru.cache_num = 0;
                    asr_proc_info.cache_10ms_stru.already_get_500ms_buf = false;
                    memcpy((asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr->data) + 4,
                        asr_proc_info.cache_10ms_stru.copy_pingpong_buf[asr_proc_info.cache_10ms_stru.copy_id],
                        CACHE_BUF_NUM * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS);
                    asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr->vadflag = true;

                    if (client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr in pcm queue lock\n", __func__);
                    } else {
                        printf("%s:%d:%s: asr in pcm queue lock\n", __FILE__, __LINE__, __func__);
                    }

                    pthread_mutex_lock(&asr_mutex_cond.asr_send_mutex);
                    send_asr_buf_queue_push(&asr_proc_info.send_asr_buf, asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr);
                    pthread_mutex_unlock(&asr_mutex_cond.asr_send_mutex);

                    if (client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s: asr in pcm queue unlock\n", __func__);
                    } else {
                        printf("%s:%d:%s: asr in pcm queue unlock\n", __FILE__, __LINE__, __func__);
                    }

                    asr_proc_info.cache_10ms_stru.send_500ms_cache_queue_ptr = NULL;

                    if (client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(asr_proc_info.sendlogobj, DEBUG, "%s:发送信号，取500ms队列,普通包", __func__);
                    } else {
                        DEBUG("%s:发送信号，取500ms队列,普通包", __func__);
                    }

                    pthread_cond_signal(&asr_mutex_cond.asr_cond_process);
                }
            }
        }
    } else {
        queue_proc:
            int ret = cache_send_buf(asr_proc_info.cache_10ms_stru.copy_pingpong_buf[asr_proc_info.cache_10ms_stru.copy_id], send_buf, asr_proc_info.cache_10ms_stru.cache_num);
            if (CACHE_BUF_NUM == ret) {
                asr_proc_info.cache_10ms_stru.send_id = asr_proc_info.cache_10ms_stru.copy_id;
                asr_proc_info.cache_10ms_stru.copy_id = ((asr_proc_info.cache_10ms_stru.copy_id + 1) % 2);
                asr_proc_info.cache_10ms_stru.cache_num = 0;
            }
    }

    return 0;
}


void *InteractiveProcForASR::thread_process(void *arg) {
    INFO("#### ASR SEND SERVER THREAD RUN! ####");

    char tmp_buf[CACHE_BUF_NUM * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS + 4] = {0};
    SEND_500MS_QUEUE tmp_queue_send_asr;
    tmp_queue_send_asr.data = tmp_buf;
    SEND_500MS_QUEUE * tmp_queue_send_asr_ptr = NULL;

    InteractiveProcForASR *obj = static_cast<InteractiveProcForASR *>(arg);
    static_obj = obj;
    obj->asr_proc_info.send_pakets_count = 0;

    SENDLOG_CLI_OBJ_PTR(obj->asr_proc_info.sendlogobj, DEBUG, "#### ASR SEND SERVER THREAD RUN! ####");

    if (asrthreadmod.send_asr_thread_stop) {
        INFO("#### ASR SEND SERVER THREAD END! ####");
        if (client_send_log_status)
            SENDLOG_CLI_OBJ_PTR(obj->asr_proc_info.sendlogobj, DEBUG, "#### ASR SEND SERVER THREAD END! ####");
        pthread_exit(NULL);
    }

    pthread_mutex_lock(&obj->asr_mutex_cond.asr_send_mutex);

    while (!asrthreadmod.send_asr_thread_stop) {
        while (!asrthreadmod.send_asr_thread_stop && (obj->asr_proc_info.start_new_session || NULL == obj->asr_proc_info.send_asr_buf )) {
            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(obj->asr_proc_info.sendlogobj, DEBUG, "waiting for send asr server signal!");
            } else {
                DEBUG("%s:waiting for send asr server signal!", __func__);
            }

            pthread_cond_wait(&obj->asr_mutex_cond.asr_cond_process, &obj->asr_mutex_cond.asr_send_mutex);

            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(obj->asr_proc_info.sendlogobj, DEBUG, "waiting for send asr server signal success!");
            } else {
                DEBUG("%s:waiting for send asr server signal success!", __func__);
            }
        }

        if (asrthreadmod.send_asr_thread_stop) {
            if (client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(obj->asr_proc_info.sendlogobj, DEBUG, "send_asr_thread_stop is true!");
            } else {
                INFO("%s:send_asr_thread_stop is true!", __func__);
            }

            break;
        }

        tmp_queue_send_asr_ptr = obj->send_asr_buf_queue_pop(&obj->asr_proc_info.send_asr_buf);
        pthread_mutex_unlock(&obj->asr_mutex_cond.asr_send_mutex);

        memcpy(tmp_queue_send_asr.data, tmp_queue_send_asr_ptr->data, CACHE_BUF_NUM * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS + 4);
        tmp_queue_send_asr.vadflag = tmp_queue_send_asr_ptr->vadflag;

        if (0 == obj->asr_proc_info.send_pakets_count) {
            obj->deinit_curl_session();
        }

        if (obj->asr_proc_info.curl_start_session) {
            obj->init_curl_session();
        }

        if (false == tmp_queue_send_asr.vadflag) {
            ++(obj->asr_proc_info.send_pakets_count);
            obj->SendDataToAsr(reinterpret_cast<char*>(tmp_queue_send_asr.data), CACHE_BUF_NUM * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS + 4, obj->asr_proc_info.send_pakets_count, 1);
            obj->asr_proc_info.send_pakets_count = 0;
        } else {
            ++(obj->asr_proc_info.send_pakets_count);
            if (-1 == obj->SendDataToAsr(reinterpret_cast<char*>(tmp_queue_send_asr.data), CACHE_BUF_NUM * SAMPLENUM * SAMPLEBYTES * IN_CHANNELS + 4, obj->asr_proc_info.send_pakets_count, 0)) {
                obj->asr_proc_info.send_pakets_count = 0;
            }
        }

        pthread_mutex_lock(&obj->asr_mutex_cond.asr_send_mutex);
        obj->send_asr_buf_queue_push(&obj->asr_proc_info.send_asr_buf_free, tmp_queue_send_asr_ptr);
    }

    pthread_mutex_unlock(&obj->asr_mutex_cond.asr_send_mutex);

    if (client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(obj->asr_proc_info.sendlogobj, DEBUG, "#### ASR SEND SERVER THREAD END! ####");
    } else {
        INFO("#### ASR SEND SERVER THREAD END! ####");
    }

    pthread_exit(NULL);
}
