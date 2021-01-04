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

#include <python2.7/Python.h>
#include "preproc.h"
#include "frame_work.h"

#define MAX_DUMP_PCM_TIME  SAMPLENUM * SAMPLEBYTES * IN_CHANNELS * 1000 * 6 * 30  // 30min

#define PY_CLASS_MODULE 1

#define DUMP_PCM 0

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

const int buf_bytesnum = SAMPLENUM * SAMPLEBYTES * IN_CHANNELS;

// for led control with python
#if PY_CLASS_MODULE
PyObject * pModule = NULL;
PyObject * pDict = NULL;
PyObject * pClassCalc = NULL;
PyObject * pInstance = NULL;
#else
PyObject * ledpython_name = NULL;
PyObject * pModule = NULL;
PyObject * pFunc_lighton = NULL;
PyObject * pFunc_lightoff = NULL;
#endif

SYSTEM_STATUS_VALUE systemmod;

#if PY_CLASS_MODULE
void ledpower_init() {
    PyObject_CallMethod(pInstance, const_cast<char *>("power_on_test"), const_cast<char *>(""));
    printf("ledpower_init.\n");
}

void ledpower_off() {
    PyObject_CallMethod(pInstance, const_cast<char *>("power_off_test"), const_cast<char *>(""));
    printf("ledpower_off.\n");
}

void ledon_dir(int dir) {
    PyObject_CallMethod(pInstance, const_cast<char *>("Led_lighton"), const_cast<char *>("i"), dir);
}

void ledoff() {
    PyObject_CallMethod(pInstance, const_cast<char *>("Led_lightoff"), const_cast<char *>(""));
}
#else
void ledon_dir(int dir) {
    int tmp = dir;
    PyObject *pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("i", tmp));
    PyEval_CallObject(pFunc_lighton, pArgs);
}

void ledoff() {
    PyEval_CallObject(pFunc_lightoff, NULL);
}
#endif

//  Initial operation
int FrameWorkProc::FrameWorkInit() {
#if __SEND_LOG_TO_SERVER__
    framework_info_stru.send_log_to_serv_obj = new SendLogProc();
    if (NULL == framework_info_stru.send_log_to_serv_obj) {
        printf("%s:%d:%s: ERROR [thread malloc error!]\n", __FILE__, __LINE__, __func__);
        framework_info_stru.client_send_log_status = false;
    } else {
        framework_info_stru.client_send_log_status = true;
    }

    if (-1 == framework_info_stru.send_log_to_serv_obj->Init()) {
        printf("%s:%d:%s: ERROR [send log Init err!]\n", __FILE__, __LINE__, __func__);
        framework_info_stru.client_send_log_status = false;
    } else {
        framework_info_stru.client_send_log_status = true;
    }
#endif

#if PY_CLASS_MODULE
    Py_Initialize();

    if (Py_IsInitialized()) {
        PyRun_SimpleString("import sys");
        PyRun_SimpleString("sys.path.append('./')");

        pModule = PyImport_ImportModule("respeaker_4mic_array_class");
        if (pModule) {
            pDict = PyModule_GetDict(pModule);
            if (!pDict) {
                printf("Cant find dictionary.\n");
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "Cant find dictionary.\n");
                return -1;
            }

            pClassCalc = PyDict_GetItemString(pDict, "cled_test");
            if (!pClassCalc) {
                printf("Cant find cled_test class.\n");
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "Cant find cled_test class.\n");
                return -1;
            }

            pInstance = PyObject_CallObject(pClassCalc, NULL);
            if (!pInstance) {
                printf("Cant find cled_test  pInstance class.\n");
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "Cant find cled_test  pInstance class.\n");
                return -1;
            }
        } else {
            printf("导入Python模块失败...\n");
            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "导入Python模块失败...\n");
            return -1;
        }
    } else {
        printf("Python环境初始化失败...\n");
        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "Python环境初始化失败...\n");
        return -1;
    }

    printf("Python环境初始化成功...\n");
    SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "Python环境初始化成功...\n");
#else
    Py_Initialize();

    if (Py_IsInitialized()) {
        PyRun_SimpleString("import sys");
        PyRun_SimpleString("sys.path.append('./')");

        ledpython_name = PyString_FromString("respeaker_4mic_array");
        if (!ledpython_name)
            printf("can't do PyString_FromString");

        pModule = PyImport_Import(ledpython_name);
        if (pModule) {
            pFunc_lighton = PyObject_GetAttrString(pModule, "Led_lighton");
            pFunc_lightoff = PyObject_GetAttrString(pModule, "Led_lightoff");
        } else {
            printf("导入Python模块失败...\n");
            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "导入Python模块失败...\n");
            return -1;
        }
    } else {
        printf("Python环境初始化失败...\n");
        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "Python环境初始化失败...\n");
        reutrn -1;
    }

    printf("Python环境初始化成功...\n");
    SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "Python环境初始化成功...\n");
#endif

    framework_info_stru.awake_count = 30;
    framework_info_stru.awake_status = 0;

    curl_global_init(CURL_GLOBAL_ALL);

    GetConfig();

    if (-1 == FileInfoInit()) {
        return -1;
    }

    if (-1 == AudioBuffInit()) {
        return -1;
    }

    if (-1 == OtherFunClassInit()) {
        return -1;
    }

    if (framework_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, " %s: frame work init success!\n", __func__);
    } else {
        printf("%s:%d %s: frame work init success!\n", __FILE__, __LINE__, __func__);
    }

    return 0;
}

//  Deinitialize
int FrameWorkProc::FrameWorkDeInit() {
#if PY_CLASS_MODULE
    if (NULL != pDict)
        Py_DECREF(pDict);

    if (NULL != pClassCalc)
        Py_DECREF(pClassCalc);

    if (NULL != pInstance)
        Py_DECREF(pInstance);

    if (NULL != pModule)
        Py_DECREF(pModule);

    Py_Finalize();
#else
    if (NULL != pFunc_lightoff)
        Py_DECREF(pFunc_lightoff);

    if (NULL != pFunc_lighton)
        Py_DECREF(pFunc_lighton);

    if (NULL != pModule)
        Py_DECREF(pModule);

    if (NULL != ledpython_name)
        Py_DECREF(ledpython_name);

    Py_Finalize();
#endif

    curl_global_cleanup();
    FileInfoDeInit();
    AudioBuffDeInit();
    OtherFunClassDeInit();

#if __SEND_LOG_TO_SERVER__
    if (NULL != framework_info_stru.send_log_to_serv_obj) {
        framework_info_stru.send_log_to_serv_obj->DeInit();
        delete framework_info_stru.send_log_to_serv_obj;
    }
#endif

    printf("%s:%s: frame work deinit success!\n", __FILE__, __func__);
    return 0;
}

void * FrameWorkProc::get_tts_res(const char * pcm_buf, int pcm_len, void * arg) {
    DEBUG("%s:into get tts res callback!\n", __func__);
    FRAME_WORK_GET_TTS_INFO *tmp = NULL;
    if (NULL != pcm_buf && 0 != pcm_len) {
        tmp = reinterpret_cast<FRAME_WORK_GET_TTS_INFO *>(arg);
        memcpy(tmp->data + 160 * 2 * 1 * 50, pcm_buf, TTS_VOICE_LEN);
        tmp->copy_len = pcm_len + 160 * 2 * 1 * 50;
        tmp->get_status = true;
    } else {
        tmp = reinterpret_cast<FRAME_WORK_GET_TTS_INFO *>(arg);
        tmp->get_status = true;
        tmp->copy_len = 0;
    }

    return nullptr;
}

int FrameWorkProc::pause() {
    framework_info_stru.tts_proc_obj->pause();
    framework_info_stru.asr_proc_obj->pause();

    return 0;
}

int FrameWorkProc::SetKugouBinding(const char * userid, const char *token_id) {
    char music_list[1024];
    memset(music_list, 0, sizeof(music_list));
    snprintf( music_list, sizeof(music_list), "XXX%s %s", userid, token_id);  //  "https://xxxx.xxxx.com.cn/musicSearch/niu/loginByToken?niuUserId=%s&niuToken=%s&deviceType=xxxx", userid, token_id);
    std::string BindingToken_strURL = reinterpret_cast<char *>(music_list);
    std::cout << "BindingToken_strURL:" << BindingToken_strURL << std::endl;
    framework_info_stru.play_obj->App_Binding_Token(BindingToken_strURL, 300, token_id);
    std::cout << "SetKugouBinding end!" << std::endl;
    FrameWorkDeInit();
    exit(0);

    return 0;
}

int FrameWorkProc::FrameWorkProcess() {
    bool vadflag = false, triggerflag = false;
    char in_buf[ASR_RESPONSE_MAX_MSG +1] = {0};
    int16_t angle = -1;
    int16_t real_angle = -1;

    struct tm * timenow = NULL;
    char timechar[256] = {0};

    const uint64_t max_file_size = MAX_DUMP_PCM_TIME;
    uint64_t record_size = 0;

    // memset(framework_info_stru.tts_txt_buf,0,TTS_TXT_LEN);
    // sprintf(framework_info_stru.tts_txt_buf,"闹钟设置成功");
    // framework_info_stru.tts_proc_obj->set_tts_info(framework_info_stru.tts_txt_buf,TTS_TXT_LEN,get_tts_res,(void *)&framework_info_stru.tts_voice_buf);

    #if PY_CLASS_MODULE
    ledpower_init();
    #endif

    if (REAL_TIME_ANGLE_LED == framework_info_stru.framework_config.doa_type) {
        set_parameters(0);
    }

    int mean_5_angle = 0;
    int tmp_count_life = 0;

    while (playthreadmod.play_thread_started) {
        ++tmp_count_life;
        if (500 == tmp_count_life) {
            tmp_count_life = 0;

            if (framework_info_stru.client_send_log_status){
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "#################### frame_work active #############");
            } else {
                printf("%s:%d:%s: frame_work active !\n", __FILE__, __LINE__, __func__);
            }
        }

        framework_info_stru.real_wakeup_status = false;
        Get_respeaker_data(AudioBuffPtr[_RECORD_8_CHANNEL], buf_bytesnum, vadflag, triggerflag, &angle);

        if (-1 != angle && REAL_TIME_ANGLE_LED == framework_info_stru.framework_config.doa_type) {
            mean_5_angle = real_angle = angle;

            if (mean_5_angle-60 < 0) {
                mean_5_angle = 300 + mean_5_angle;
            } else {
                mean_5_angle = mean_5_angle - 60;
            }

            #if PY_CLASS_MODULE
            ledon_dir(mean_5_angle);
            #else
            ledon_dir(mean_5_angle);
            #endif
        }

#if DUMP_PCM
        if (max_file_size <= record_size) {
            fclose(AudioFileFp[_RECORD_8_CHANNEL]);
            AudioFileFp[_RECORD_8_CHANNEL] = NULL;

            AudioFileFp[_RECORD_8_CHANNEL] = fopen(DUMP_PCM_8_CHANNEL_FILE, "wb+");
            if (NULL == AudioFileFp[_RECORD_8_CHANNEL]) {
                printf("%s:%d:%s: ERROR [record pcm file open fail!]\n", __FILE__, __LINE__, __func__);
                break;
            }
            record_size = 0;
            DEBUG("%s:%d:%s: record pcm file open ,Re-record 30min voice!\n", __FILE__, __LINE__, __func__);
        }

        record_size += buf_bytesnum;
        fwrite(AudioBuffPtr[_RECORD_8_CHANNEL], sizeof(char), buf_bytesnum, AudioFileFp[_RECORD_8_CHANNEL]);
#endif

        framework_info_stru.real_wakeup_status = get_real_awake_status(triggerflag);
        framework_info_stru.asr_proc_obj->set_send_asr_info(AudioBuffPtr[_RECORD_8_CHANNEL], buf_bytesnum, vadflag);

        if (1 == framework_info_stru.clock_voice_obj->time_count_proc()) {
            if (framework_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "闹钟时间到啦!");
            } else {
                DEBUG("闹钟时间到啦!");
            }

            if (1 == framework_info_stru.clock_voice_obj->play_select()) {
                systemmod.sys_type = SYS_SET_CLOCK;
                systemmod.sys_old_type = SYS_SET_CLOCK;
                framework_info_stru.play_obj->set_clock_tts_voice_info(framework_info_stru.clock_voice_obj->clock_voice_stru.tts_voice_2_channel_buf,
                    framework_info_stru.clock_voice_obj->clock_voice_stru.clock_voice_channel_2_len);
                framework_info_stru.play_obj->SendSig(PLAY_CLOCK_TTS);

                if (framework_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "发送信号让播放线程播放clock voice tts!");
                } else {
                    DEBUG("发送信号让播放线程播放clock voice tts!");
                }
            } else {
                systemmod.sys_type = SYS_SET_CLOCK;
                systemmod.sys_old_type = SYS_SET_CLOCK;
                framework_info_stru.play_obj->SendSig(PLAY_CLOCK_TTS_FILE);

                if (framework_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "发送信号让播放线程播放clock voice LOCAL!");
                } else {
                    DEBUG("发送信号让播放线程播放clock voice LOCAL!");
                }
            }

            framework_info_stru.clock_voice_obj->set_clock_voice_info_deinit();
        }

        if (framework_info_stru.real_wakeup_status) {
            // pause earlier operations
            systemmod.sys_type = SYS_VOICE_RECOGNIZE;
            asrthreadmod.global_asr_res = false;
            pause();

            if (-1 != angle && AWAKE_ANGLE_LED == framework_info_stru.framework_config.doa_type) {
                mean_5_angle = real_angle = angle;
                if (mean_5_angle-60 < 0) {
                    mean_5_angle = 300 + mean_5_angle;
                } else {
                    mean_5_angle = mean_5_angle - 60;
                }

                #if PY_CLASS_MODULE
                ledon_dir(mean_5_angle);
                #else
                ledon_dir(mean_5_angle);
                #endif
            }

            if (framework_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG,
                    "\n\n%s:%d:%s:the current angle is:%d mean_5_angle:%d \n", __FILE__, __LINE__, __func__, angle, mean_5_angle);
            } else {
                printf("\n\n%s:%d:%s:the current angle is:%d mean_5_angle:%d \n", __FILE__, __LINE__, __func__, angle, mean_5_angle);
            }

            real_angle = -1;

            framework_info_stru.play_obj->SendSig(PLAY_AWAKE);

            if (framework_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "系统进入语音识别状态");
            } else {
                DEBUG("%s:系统进入语音识别状态", __func__);
            }

            time_t second = time(0);
            timenow = localtime(&second);
            memset(timechar, 0, sizeof(timechar));
            snprintf(timechar, sizeof(timechar), "%d-%d-%d %d:%d:%d", timenow->tm_year+1900, timenow->tm_mon+1,
                        timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

            if (framework_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:now time is %s", __func__, timechar);
            } else {
                DEBUG("%s:now time is %s", __func__ , timechar);
            }
        } else {
            switch (systemmod.sys_type) {
            case SYS_VOICE_RECOGNIZE:
                if (asrthreadmod.global_asr_res) {
                    asrthreadmod.global_asr_res = false;
                    framework_info_stru.asr_proc_obj->GetAsrResult(in_buf);
                    if (0 == analysis_asr_result(in_buf)) {
                        switch (asrthreadmod.asr_mod) {
                        case ASR_MUSIC_START:
                            asrthreadmod.asr_mod = ASR_DEFAULT;
                            systemmod.sys_type = SYS_MUSIC_START;
                            systemmod.sys_old_type = systemmod.sys_type;
                            framework_info_stru.tts_proc_obj->set_tts_info(framework_info_stru.tts_txt_buf, TTS_TXT_LEN, get_tts_res,
                                reinterpret_cast<void *>(&framework_info_stru.tts_voice_buf));

                            if (framework_info_stru.client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:系统由语音识别状态转换为播放音乐状态", __func__);
                            } else {
                                DEBUG("%s:系统由语音识别状态转换为播放音乐状态", __func__);
                            }

                            break;

                        case ASR_MUSIC_STOP:
                            asrthreadmod.asr_mod = ASR_DEFAULT;
                            systemmod.sys_type = SYS_MUSIC_STOP;
                            systemmod.sys_old_type = SYS_DEFAULT;
                            framework_info_stru.play_obj->SendSig(STOP_MUSIC);

                            if (framework_info_stru.client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:系统由语音识别状态转换为停止播放音乐状态", __func__);
                            } else {
                                DEBUG("%s:系统由语音识别状态转换为停止播放音乐状态", __func__);
                            }

                            break;

                        case ASR_GET_TTS:
                            asrthreadmod.asr_mod = ASR_DEFAULT;
                            systemmod.sys_type = SYS_TTS;
                            systemmod.sys_old_type = SYS_DEFAULT;
                            framework_info_stru.tts_proc_obj->set_tts_info(framework_info_stru.tts_txt_buf, TTS_TXT_LEN, get_tts_res,
                                reinterpret_cast<void *>(&framework_info_stru.tts_voice_buf));

                            if (framework_info_stru.client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:获取聊天结果 转换为语音 get tts", __func__);
                            } else {
                                DEBUG("%s:获取聊天结果 转换为语音 get tts", __func__);
                            }

                            break;

                        case ASR_NO_HEAR:
                            asrthreadmod.asr_mod = ASR_DEFAULT;
                            systemmod.sys_type = SYS_NO_HEAR;
                            framework_info_stru.play_obj->SendSig(PLAY_NO_HEAR);

                            if (framework_info_stru.client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:系统由语音识别状态转换为播放no hear voice状态", __func__);
                            } else {
                                DEBUG("%s:系统由语音识别状态转换为播放no hear voice状态", __func__);
                            }

                            break;

                        case ASR_SET_CLOCK:
                            asrthreadmod.asr_mod = ASR_DEFAULT;
                            systemmod.sys_type = SYS_SET_CLOCK;
                            framework_info_stru.clock_voice_obj->send_clock_sig();
                            framework_info_stru.play_obj->SendSig(PLAY_CLOCK_RES_OK);

                            if (framework_info_stru.client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:系统由语音识别状态转换为SET CLOCK VOICE 状态", __func__);
                            } else {
                                DEBUG("%s:系统由语音识别状态转换为SET CLOCK VOICE 状态", __func__);
                            }

                            break;

                        case ASR_NOT_UNDERSTAND:
                            asrthreadmod.asr_mod = ASR_DEFAULT;
                            systemmod.sys_type = SYS_NOT_UNDERSTAND;
                            systemmod.sys_old_type = SYS_DEFAULT;
                            framework_info_stru.play_obj->SendSig(PLAY_NOT_UNDERSTAND);

                            if (framework_info_stru.client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:系统由语音识别状态转换为播放not understand voice状态", __func__);
                            } else {
                                DEBUG("%s:系统由语音识别状态转换为播放not understand voice状态", __func__);
                            }

                            break;

                        case ASR_NET_ERR:
                            asrthreadmod.asr_mod = ASR_DEFAULT;
                            systemmod.sys_type = SYS_NET_ERR;
                            systemmod.sys_old_type = SYS_DEFAULT;
                            framework_info_stru.play_obj->SendSig(PLAY_NET_ERR);

                            if (framework_info_stru.client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:系统由语音识别状态转换为播放net err voice 状态", __func__);
                            } else {
                                DEBUG("%s:系统由语音识别状态转换为播放net err voice 状态", __func__);
                            }

                            break;

                        case ASR_SET_VOL_UP:
                            asrthreadmod.asr_mod = ASR_DEFAULT;
                            systemmod.sys_type = SYS_SET_VOLUME;
                            framework_info_stru.ctl_vol_obj->SendSig(SET_VOLUME_UP);

                            if (framework_info_stru.client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "系统由语音识别调高音量");
                            } else {
                                DEBUG("%s:系统由语音识别调高音量", __func__);
                            }

                            break;

                        case ASR_SET_VOL_DOWN:
                            asrthreadmod.asr_mod = ASR_DEFAULT;
                            systemmod.sys_type = SYS_SET_VOLUME;
                            framework_info_stru.ctl_vol_obj->SendSig(SET_VOLUME_DOWN);

                            if (framework_info_stru.client_send_log_status) {
                                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "系统由语音识别调低音量");
                            } else {
                                DEBUG("%s:系统由语音识别调低音量", __func__);
                            }

                            break;

                        default:
                            break;
                        }
                    }
                }
                break;

            case SYS_MUSIC_START:
                if (framework_info_stru.tts_voice_buf.get_status) {
                    if (framework_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "SYS_MUSIC_START in frame_work while");
                    } else {
                        DEBUG("SYS_MUSIC_START in frame_work while");
                    }

                    framework_info_stru.tts_voice_buf.get_status = false;

                    if (framework_info_stru.tts_voice_buf.copy_len) {
                        framework_info_stru.play_obj->set_tts_voice_info(framework_info_stru.tts_voice_buf.data, framework_info_stru.tts_voice_buf.copy_len);
                        framework_info_stru.play_obj->SendSig(PLAY_MUSIC);

                        if (framework_info_stru.client_send_log_status) {
                            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "发送信号让播放线程播放音乐");
                        } else {
                            DEBUG("发送信号让播放线程播放音乐");
                        }
                    } else {
                        systemmod.sys_type = SYS_NET_ERR;
                        systemmod.sys_old_type = SYS_DEFAULT;
                        framework_info_stru.play_obj->SendSig(PLAY_NET_ERR);

                        if (framework_info_stru.client_send_log_status) {
                            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:系统由语音识别状态转换为播放net err voice 状态", __func__);
                        } else {
                            DEBUG("%s:系统由语音识别状态转换为播放net err voice 状态", __func__);
                        }
                    }
                }
                break;

            case SYS_MUSIC_STOP:
                break;

            case SYS_TTS:
                if (framework_info_stru.tts_voice_buf.get_status) {
                    if (framework_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "SYS_TTS in frame_work while");
                    } else {
                        DEBUG("SYS_TTS in frame_work while");
                    }

                    framework_info_stru.tts_voice_buf.get_status = false;

                    if (framework_info_stru.tts_voice_buf.copy_len) {
                        framework_info_stru.play_obj->set_tts_voice_info(framework_info_stru.tts_voice_buf.data, framework_info_stru.tts_voice_buf.copy_len);
                        framework_info_stru.play_obj->SendSig(PLAY_TTS);

                        if (framework_info_stru.client_send_log_status) {
                            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "发送信号让播放线程播放chat tts!");
                        } else {
                            DEBUG("发送信号让播放线程播放chat tts!");
                        }
                    } else {
                        systemmod.sys_type = SYS_NET_ERR;
                        systemmod.sys_old_type = SYS_DEFAULT;
                        framework_info_stru.play_obj->SendSig(PLAY_NET_ERR);

                        if (framework_info_stru.client_send_log_status) {
                            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:系统由语音识别状态转换为播放net err voice 状态", __func__);
                        } else {
                            DEBUG("%s:系统由语音识别状态转换为播放net err voice 状态", __func__);
                        }
                    }
                }
                break;

            case SYS_NET_ERR:
                break;

            case SYS_NOT_UNDERSTAND:
                break;

            case SYS_SET_CLOCK:
                switch (systemmod.sys_old_type) {
                case SYS_MUSIC_START:
                    systemmod.sys_type = systemmod.sys_old_type;
                    framework_info_stru.tts_proc_obj->set_tts_info(framework_info_stru.tts_txt_buf, TTS_TXT_LEN, get_tts_res,
                        reinterpret_cast<void *>(&framework_info_stru.tts_voice_buf));

                    if (framework_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:恢复播放音乐状态！", __func__);
                    } else {
                        DEBUG("%s:恢复播放音乐状态！", __func__);
                    }

                    break;
                default:
                    break;
                }
                break;


            case SYS_NO_HEAR:
                switch (systemmod.sys_old_type) {
                case SYS_MUSIC_START:
                    systemmod.sys_type = systemmod.sys_old_type;
                    framework_info_stru.tts_proc_obj->set_tts_info(framework_info_stru.tts_txt_buf, TTS_TXT_LEN, get_tts_res,
                        reinterpret_cast<void *>(&framework_info_stru.tts_voice_buf));

                    if (framework_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:恢复播放音乐状态！", __func__);
                    } else {
                        DEBUG("%s:恢复播放音乐状态！", __func__);
                    }

                    break;
                default:
                    break;
                }
                break;

            case SYS_SET_VOLUME:
                switch (systemmod.sys_old_type) {
                case SYS_MUSIC_START:
                    systemmod.sys_type = systemmod.sys_old_type;
                    framework_info_stru.tts_proc_obj->set_tts_info(framework_info_stru.tts_txt_buf, TTS_TXT_LEN, get_tts_res,
                        reinterpret_cast<void *>(&framework_info_stru.tts_voice_buf));

                    if (framework_info_stru.client_send_log_status) {
                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "恢复播放音乐状态！");
                    } else {
                        DEBUG("%s:恢复播放音乐状态！", __func__);
                    }
                    break;
                default:
                    break;
                }
                break;

            default:
                break;
            }
        }
    }

    #if PY_CLASS_MODULE
    ledpower_off();
    #endif
    return 0;
}

int FrameWorkProc::analysis_asr_result(char *in_buf) {
    char focus_fg[10] = {0};
    char rawtext_buf[2048] = {0};
    cJSON *_ret_parse = cJSON_Parse(reinterpret_cast<char *>(in_buf));
    if (!_ret_parse) {
        if (framework_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:%d:%s:cJSON parse erreor! [%s]\n",
                __FILE__, __LINE__, __func__, cJSON_GetErrorPtr());
        } else {
            printf("%s:%d:%s:cJSON parse erreor! [%s]\n", __FILE__, __LINE__, __func__, cJSON_GetErrorPtr());
        }

        if (ASR_NET_ERR != asrthreadmod.asr_mod) {
            asrthreadmod.asr_mod = ASR_NO_HEAR;
        }
    } else {
        cJSON * raw_text = cJSON_GetObjectItem(_ret_parse, "rawText");
        if (NULL != raw_text) {
            if (framework_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "rawText:%s", raw_text->valuestring);
            } else {
                INFO("rawText:%s", raw_text->valuestring);
            }

            snprintf(rawtext_buf, sizeof(rawtext_buf), "%s", raw_text->valuestring);

            if (strlen(rawtext_buf) != 0) {
                if (0 != framework_info_stru.clock_voice_obj->AnalysisAsrResult(rawtext_buf)) {
                    if (strstr(rawtext_buf, "停止播放") != NULL) {
                        asrthreadmod.asr_mod = ASR_MUSIC_STOP;

                        if (framework_info_stru.client_send_log_status) {
                            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:系统状态应切换置停止播放音乐！", __func__);
                        } else {
                            DEBUG("%s:系统状态应切换置停止播放音乐！", __func__);
                        }
                    } else if (strstr(rawtext_buf, "调高音量") != NULL) {
                        asrthreadmod.asr_mod = ASR_SET_VOL_UP;
                    } else if (strstr(rawtext_buf, "调低音量") != NULL)  {
                        asrthreadmod.asr_mod = ASR_SET_VOL_DOWN;
                    } else {
                        cJSON * result = cJSON_GetObjectItem(_ret_parse, "result");
                        if (NULL != result) {
                            cJSON *focus = cJSON_GetObjectItem(result, "focus");
                            if (NULL != focus) {
                                memset(focus_fg, 0, sizeof(focus_fg));
                                snprintf(focus_fg, sizeof(focus_fg), "%s", focus->valuestring);

                                if (framework_info_stru.client_send_log_status) {
                                    SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:focus_fg :%s\n", __func__, focus_fg);
                                } else {
                                    INFO("%s:focus_fg :%s\n", __func__, focus_fg);
                                }

                                if (strcmp(focus_fg, "music") == 0) {
                                    cJSON * object = cJSON_GetObjectItem(result, "object");
                                    char * object_Data = cJSON_Print(object);

                                    if (framework_info_stru.client_send_log_status) {
                                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:object_Data :%s", __func__, object_Data);
                                    } else {
                                        INFO("%s:object_Data :%s", __func__, object_Data);
                                    }

                                    if (strstr(object_Data, "artist") != NULL) {
                                        cJSON *_ret_getartist = cJSON_GetObjectItem(object, "artist");
                                        memset(framework_info_stru.artist, 0, framework_info_stru.artist_len);
                                        snprintf(framework_info_stru.artist, sizeof(framework_info_stru.artist), "%s", _ret_getartist->valuestring);

                                        if (framework_info_stru.client_send_log_status) {
                                            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:歌手：%s", __func__, framework_info_stru.artist);
                                        } else {
                                            INFO("%s:歌手：%s", __func__, framework_info_stru.artist);
                                        }
                                    } else {
                                        memset(framework_info_stru.artist, 0, framework_info_stru.artist_len);

                                        if (framework_info_stru.client_send_log_status) {
                                            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, " 清空 artist!,artist:%s,artist_len:%d",
                                                framework_info_stru.artist, framework_info_stru.artist_len);
                                        } else {
                                            DEBUG(" 清空 artist!,artist:%s,artist_len:%d", framework_info_stru.artist, framework_info_stru.artist_len);
                                        }
                                    }

                                    if (strstr(object_Data, "song") != NULL) {
                                        cJSON * _ret_getsong = cJSON_GetObjectItem(object, "song");
                                        memset(framework_info_stru.song_name, 0, framework_info_stru.song_name_len);
                                        snprintf(framework_info_stru.song_name, sizeof(framework_info_stru.song_name), "%s", _ret_getsong->valuestring);

                                        if (framework_info_stru.client_send_log_status) {
                                            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:歌曲名：%s", __func__, framework_info_stru.song_name);
                                        } else {
                                            INFO("%s:歌曲名：%s", __func__, framework_info_stru.song_name);
                                        }
                                    } else {
                                        memset(framework_info_stru.song_name, 0, framework_info_stru.song_name_len);

                                        if (framework_info_stru.client_send_log_status) {
                                            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, " 清空 song_name!,song_name:%s,song_name_len:%d",
                                                framework_info_stru.song_name, framework_info_stru.song_name_len);
                                        } else {
                                            DEBUG(" 清空 song_name!,song_name:%s,song_name_len:%d", framework_info_stru.song_name, framework_info_stru.song_name_len);
                                        }
                                    }

                                    if ((strlen(framework_info_stru.artist) == 0) && (strlen(framework_info_stru.song_name) == 0)) {
                                        asrthreadmod.asr_mod = ASR_MUSIC_START;
                                        memset(framework_info_stru.tts_txt_buf, 0, sizeof(framework_info_stru.tts_txt_buf));
                                        snprintf(framework_info_stru.tts_txt_buf, sizeof(framework_info_stru.tts_txt_buf), "%s", "没听到你说的歌曲是什么!为您随机播放歌曲!");
                                    } else {
                                        asrthreadmod.asr_mod = ASR_MUSIC_START;
                                        memset(framework_info_stru.tts_txt_buf, 0, sizeof(framework_info_stru.tts_txt_buf));

                                        if ((0 != strlen(framework_info_stru.artist)) && (0 != strlen(framework_info_stru.song_name))) {
                                            snprintf(framework_info_stru.tts_txt_buf, sizeof(framework_info_stru.tts_txt_buf), "将为您播放%s的%s!",
                                                    framework_info_stru.artist, framework_info_stru.song_name);
                                        } else if (0 != strlen(framework_info_stru.artist)) {
                                            snprintf(framework_info_stru.tts_txt_buf, sizeof(framework_info_stru.tts_txt_buf), "将为您播放%s的歌曲!", framework_info_stru.artist);
                                        } else if (0 != strlen(framework_info_stru.song_name)) {
                                            snprintf(framework_info_stru.tts_txt_buf, sizeof(framework_info_stru.tts_txt_buf), "将为您播放歌曲%s!", framework_info_stru.song_name);
                                        }
                                    }

                                    if (framework_info_stru.client_send_log_status) {
                                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:TTS TXT:[%s]", __func__, framework_info_stru.tts_txt_buf);
                                    } else {
                                        INFO("%s:TTS TXT:[%s]", __func__, framework_info_stru.tts_txt_buf);
                                    }

                                    framework_info_stru.play_obj->set_music_info(framework_info_stru.song_name, framework_info_stru.song_name_len,
                                                                                    framework_info_stru.artist, framework_info_stru.artist_len);
                                } else if (strcmp(focus_fg, "all") == 0) {
                                    cJSON * object = cJSON_GetObjectItem(result, "object");
                                    char *object_Data = cJSON_Print(object);

                                    if (framework_info_stru.client_send_log_status) {
                                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:object_Data :%s", __func__, object_Data);
                                    } else {
                                        INFO("%s:object_Data :%s", __func__, object_Data);
                                    }

                                    cJSON * operation = cJSON_GetObjectItem(result, "operation");
                                    char * operation_data = cJSON_Print(operation);

                                    if (framework_info_stru.client_send_log_status) {
                                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:operation_data :%s", __func__, operation_data);
                                    } else {
                                        INFO("%s:operation_data :%s", __func__, operation_data);
                                    }

                                    if (strstr(operation_data, "chat") != NULL) {
                                        asrthreadmod.asr_mod = ASR_GET_TTS;
                                        memset(framework_info_stru.tts_txt_buf, 0, sizeof(framework_info_stru.tts_txt_buf));
                                        snprintf(framework_info_stru.tts_txt_buf, sizeof(framework_info_stru.tts_txt_buf), "%s", object_Data);

                                        if (framework_info_stru.client_send_log_status) {
                                            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:TTS TXT:[%s]", __func__, framework_info_stru.tts_txt_buf);
                                        } else {
                                            INFO("%s:TTS TXT:[%s]", __func__, framework_info_stru.tts_txt_buf);
                                        }
                                    } else {
                                        if ( ASR_NET_ERR != asrthreadmod.asr_mod ) {
                                            asrthreadmod.asr_mod = ASR_NOT_UNDERSTAND;
                                        }
                                    }
                                } else if (strcmp(focus_fg, "chat") == 0) {
                                    cJSON * object = cJSON_GetObjectItem(result, "object");
                                    char *object_Data = cJSON_Print(object);

                                    if (framework_info_stru.client_send_log_status) {
                                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:object_Data :%s", __func__, object_Data);
                                    } else {
                                        INFO("%s:object_Data :%s", __func__, object_Data);
                                    }

                                    cJSON *speechReply_obj = cJSON_GetObjectItem(object, "speechReply");
                                    char *object_speech_data = cJSON_Print(speechReply_obj);

                                    if (framework_info_stru.client_send_log_status) {
                                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:speechReply :%s", __func__, object_speech_data);
                                    } else {
                                        INFO("%s:speechReply :%s", __func__, object_speech_data);
                                    }

                                    cJSON * operation = cJSON_GetObjectItem(result, "operation");
                                    char * operation_data = cJSON_Print(operation);

                                    if (framework_info_stru.client_send_log_status) {
                                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:operation_data :%s", __func__, operation_data);
                                    } else {
                                        INFO("%s:operation_data :%s", __func__, operation_data);
                                    }

                                    if (strstr(operation_data, "after3rd") != NULL) {
                                        if (framework_info_stru.client_send_log_status) {
                                            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:模糊问答:[%s]", __func__, object_speech_data);
                                        } else {
                                            INFO("%s:模糊问答:[%s]", __func__, object_speech_data);
                                        }
                                    } else {
                                        if (framework_info_stru.client_send_log_status) {
                                            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:精确问答:[%s]", __func__, object_speech_data);
                                        } else {
                                            INFO("%s:精确问答:[%s]", __func__, object_speech_data);
                                        }
                                    }

                                    asrthreadmod.asr_mod = ASR_GET_TTS;
                                    memset(framework_info_stru.tts_txt_buf, 0, sizeof(framework_info_stru.tts_txt_buf));
                                    snprintf(framework_info_stru.tts_txt_buf, sizeof(framework_info_stru.tts_txt_buf), "%s", object_speech_data);

                                    if (framework_info_stru.client_send_log_status) {
                                        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:TTS TXT:[%s]", __func__, framework_info_stru.tts_txt_buf);
                                    } else {
                                        INFO("%s:TTS TXT:[%s]", __func__, framework_info_stru.tts_txt_buf);
                                    }
                                } else if (strcmp(focus_fg, "systemSet") == 0) {
                                    cJSON * object = cJSON_GetObjectItem(result, "object");
                                    char *object_Data = cJSON_Print(object);

                                    INFO("%s:object_Data :%s", __func__, object_Data);

                                    cJSON * operation = cJSON_GetObjectItem(result, "operation");
                                    char * operation_data = cJSON_Print(operation);

                                    INFO("%s:operation_data :%s", __func__, operation_data);

                                    if (strstr(operation_data, "SoundMedia_Up") != NULL) {
                                        asrthreadmod.asr_mod = ASR_SET_VOL_UP;
                                        INFO("%s:SET VOLUME UP", __func__);
                                    } else if (strstr(operation_data, "SoundMedia_Down") != NULL) {
                                        asrthreadmod.asr_mod = ASR_SET_VOL_DOWN;
                                        INFO("%s:SET VOLUME DOWN", __func__);
                                    } else {
                                        if ( ASR_NET_ERR != asrthreadmod.asr_mod ) {
                                            asrthreadmod.asr_mod = ASR_NOT_UNDERSTAND;
                                        }
                                    }
                                } else {
                                    if (ASR_NET_ERR != asrthreadmod.asr_mod) {
                                        asrthreadmod.asr_mod = ASR_NOT_UNDERSTAND;
                                    }
                                }
                            } else {
                                if (ASR_NET_ERR != asrthreadmod.asr_mod) {
                                    asrthreadmod.asr_mod = ASR_NOT_UNDERSTAND;
                                } else {
                                   framework_info_stru.play_obj->SendSig(PLAY_CLOCK_RES_OK);
                                }
                            }
                        } else {
                            if (ASR_NET_ERR != asrthreadmod.asr_mod) {
                                asrthreadmod.asr_mod = ASR_NOT_UNDERSTAND;
                            }
                        }
                    }
                } else {
                    if (1 == framework_info_stru.clock_voice_obj->set_clock_voice_info_init()) {
                        asrthreadmod.asr_mod = ASR_SET_CLOCK;
                    }
                }
            } else {
                if (ASR_NET_ERR != asrthreadmod.asr_mod) {
                    asrthreadmod.asr_mod = ASR_NO_HEAR;
                }
            }
        } else {
            if (ASR_NET_ERR != asrthreadmod.asr_mod) {
                asrthreadmod.asr_mod = ASR_NO_HEAR;
            }
        }
    }

    if (framework_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:解析完成！", __func__);
    } else {
        DEBUG("%s:解析完成！", __func__);
    }

    cJSON_Delete(_ret_parse);
    if (ASR_DEFAULT != asrthreadmod.asr_mod) {
        return 0;
    } else {
        return -1;
    }
}

bool FrameWorkProc::get_real_awake_status(bool &kwsflag) {
    if (kwsflag) {
        framework_info_stru.awake_count = 30;
        if (0 == framework_info_stru.awake_status) {
            framework_info_stru.awake_status = 1;
            if (framework_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s: real wake up!\n",  __func__);
            } else {
                printf("%s:%d:%s: real wake up!\n", __FILE__, __LINE__, __func__);
            }

            return true;
        }
    } else {
        if (1 == framework_info_stru.awake_status) {
            framework_info_stru.awake_count--;
            if (0 == framework_info_stru.awake_count) {
                framework_info_stru.awake_status = 0;
            }
        }
    }

    return false;
}

int FrameWorkProc::Get_respeaker_data(char * audio_buf, const int &read_buf_len, bool &vadflag, bool &kwsflag, int16_t *angle) {
    Preproc_mic_array_read(reinterpret_cast<void *>(audio_buf), read_buf_len, &kwsflag, &vadflag, angle, 8);
    return 0;
}

int FrameWorkProc::AnalysisConfigInfo(const char * jchar) {
    if (framework_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "INTO ANALYSIS CONFIG FILE JSON STRING!\n");
    } else {
        printf("%s:%d:%s:INTO ANALYSIS CONFIG FILE JSON STRING!\n", __FILE__, __LINE__, __func__);
    }

    cJSON *_ret_parse = cJSON_Parse(jchar);
    if (!_ret_parse) {
        if (framework_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:cJSON parse erreor! [%s]\n", __func__, cJSON_GetErrorPtr());
        } else {
            printf("%s:%d:%s:cJSON parse erreor! [%s]\n", __FILE__, __LINE__, __func__, cJSON_GetErrorPtr());
        }
        return -1;
    } else {
        cJSON * config = cJSON_GetObjectItem(_ret_parse, "config");
        if (NULL != config) {
            int array_size = cJSON_GetArraySize(config);
            cJSON *item = NULL;
            char *object_Data = NULL;
            for (int i = 0; i < array_size; i++) {
                item = cJSON_GetArrayItem(config, i);
                cJSON * doa_type = cJSON_GetObjectItem(item, "DOA_TYPE");
                object_Data = cJSON_Print(doa_type);

                if (framework_info_stru.client_send_log_status) {
                    SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s:doa_type :%s", __func__, object_Data);
                } else {
                    INFO("%s:doa_type :%s", __func__, object_Data);
                }
            }

            if (strstr(object_Data, "1") != NULL) {
                framework_info_stru.framework_config.doa_type = 1;
            }
        }
    }

    return 0;
}

int FrameWorkProc::GetConfig() {
    if (framework_info_stru.client_send_log_status) {
        SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "INTO ANALYSIS CONFIG FILE \n");
    } else {
        printf("%s:%d:%s:INTO ANALYSIS CONFIG FILE \n", __FILE__, __LINE__, __func__);
    }

    char *content = NULL;
    FILE *config_file = fopen(CONFIG_FILE, "rb");
    if (NULL == config_file) {
        if (framework_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s: ERROR [open %s fail!]\n", __func__, CONFIG_FILE);
        } else {
            printf("%s:%d:%s: ERROR [open %s fail!]\n", __FILE__, __LINE__, __func__, CONFIG_FILE);
        }
        return -1;
    }

    fseek(config_file, 0, SEEK_END);
    int len = ftell(config_file);
    fseek(config_file, 0, SEEK_SET);

    content = reinterpret_cast<char*>(malloc(len+1));
    fread(content, 1, len, config_file);
    fclose(config_file);
    AnalysisConfigInfo(content);
    free(content);
    return 0;
}

int FrameWorkProc::FileInfoInit() {
#if DUMP_PCM
    AudioFileFp[_RECORD_8_CHANNEL] = fopen(DUMP_PCM_8_CHANNEL_FILE, "wb+");
    // AudioFileFp[_RECORD_8_CHANNEL] = fopen(DUMP_PCM_8_CHANNEL_FILE,"ab");
    if (NULL == AudioFileFp[_RECORD_8_CHANNEL]) {
        printf("%s:%d:%s: ERROR [record pcm file open fail!]\n", __FILE__, __LINE__, __func__);
        return -1;
    }
#endif
    return 0;
}

int FrameWorkProc::FileInfoDeInit() {
#if DUMP_PCM
    if (AudioFileFp[_RECORD_8_CHANNEL]) {
        fclose(AudioFileFp[_RECORD_8_CHANNEL]);
        AudioFileFp[_RECORD_8_CHANNEL] = NULL;
    }
#endif

    return 0;
}

int FrameWorkProc::AudioBuffInit() {
    AudioBuffPtr[_RECORD_8_CHANNEL] = reinterpret_cast<char *>(malloc(buf_bytesnum));
    if (NULL == AudioBuffPtr[_RECORD_8_CHANNEL]) {
        if (framework_info_stru.client_send_log_status) {
            SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "%s: ERROR [malloc audio buf error!]\n",  __func__);
        } else {
            printf("%s:%d:%s: ERROR [malloc audio buf error!]\n", __FILE__, __LINE__, __func__);
        }

        return -1;
    }

    bzero(AudioBuffPtr[_RECORD_8_CHANNEL], buf_bytesnum);

    return 0;
}

int FrameWorkProc::AudioBuffDeInit() {
    if (AudioBuffPtr[_RECORD_8_CHANNEL]) {
        free(AudioBuffPtr[_RECORD_8_CHANNEL]);
        AudioBuffPtr[_RECORD_8_CHANNEL] = NULL;
    }

    return 0;
}

int FrameWorkProc::OtherFunClassInit() {
    Preproc_mic_array_init();

    if (!playthreadmod.play_thread_started) {
        framework_info_stru.play_obj = new Play_Audio_Data();
        if (NULL == framework_info_stru.play_obj) {
            if (framework_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "ERROR [playback thread malloc error!]");
            } else {
                printf("%s:%d:%s: ERROR [playback thread malloc error!]\n", __FILE__, __LINE__, __func__);
            }

            return -1;
        }

        playthreadmod.play_thread_stop = false;

        if (-1 == framework_info_stru.play_obj->Init()) {
            return -1;
        }

        playthreadmod.play_thread_started = true;
    }

    if (!asrthreadmod.send_asr_thread_started) {
        framework_info_stru.asr_proc_obj = new InteractiveProcForASR();
        if (NULL == framework_info_stru.asr_proc_obj) {
            if (framework_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "ERROR [send asr thread malloc error!]");
            } else {
                printf("%s:%d:%s: ERROR [send asr thread malloc error!]\n", __FILE__, __LINE__, __func__);
            }
            return -1;
        }

        asrthreadmod.send_asr_thread_stop = false;

        if (-1 == framework_info_stru.asr_proc_obj->Init()) {
            return -1;
        }

        asrthreadmod.send_asr_thread_started = true;
    }

    if (!ttsthreadmod.tts_thread_start) {
        framework_info_stru.tts_proc_obj = new TtsProc();
        if (NULL == framework_info_stru.tts_proc_obj) {
            if (framework_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "ERROR [send asr thread malloc error!]");
            } else {
                printf("%s:%d:%s: ERROR [send asr thread malloc error!]\n", __FILE__, __LINE__, __func__);
            }
            return -1;
        }

        ttsthreadmod.tts_thread_stop = false;

        if (-1 == framework_info_stru.tts_proc_obj->Init()) {
            return -1;
        }

        ttsthreadmod.tts_thread_start = true;
    }

    if (!clockvoicethreadmod.clock_voice_thread_start) {
        framework_info_stru.clock_voice_obj = new ClockVoiceProc();
        if (NULL == framework_info_stru.clock_voice_obj) {
            if (framework_info_stru.client_send_log_status) {
                SENDLOG_CLI_OBJ_PTR(framework_info_stru.send_log_to_serv_obj, DEBUG, "ERROR [send clock voice thread malloc error!]");
            } else {
                printf("%s:%d:%s: ERROR [send clock voice thread malloc error!]\n", __FILE__, __LINE__, __func__);
            }
            return -1;
        }
        clockvoicethreadmod.clock_voice_thread_stop = false;
        if (-1 == framework_info_stru.clock_voice_obj->Init()) {
            return -1;
        }
        clockvoicethreadmod.clock_voice_thread_start = true;
    }

    if (!volumecontrolthreadmod.volume_control_thread_start) {
        framework_info_stru.ctl_vol_obj = new FrameworkVolueProc();
        if (NULL == framework_info_stru.ctl_vol_obj) {
            printf("%s:%d:%s: ERROR [volume control thread malloc error!]\n", __FILE__, __LINE__, __func__);
            return -1;
        }
        volumecontrolthreadmod.volume_control_thread_stop = false;
        if (-1 == framework_info_stru.ctl_vol_obj->Init()) {
            return -1;
        }
        volumecontrolthreadmod.volume_control_thread_start = true;
    }

    return 0;
}

int FrameWorkProc::OtherFunClassDeInit() {
    Preproc_mic_array_deinit();
    playthreadmod.play_thread_stop = true;

    if (NULL != framework_info_stru.play_obj) {
        if (playthreadmod.play_thread_started) {
            framework_info_stru.play_obj->DeInit();
            playthreadmod.play_thread_started = false;
        }
        delete framework_info_stru.play_obj;
    }

    asrthreadmod.send_asr_thread_stop = true;

    if (NULL != framework_info_stru.asr_proc_obj) {
        if (asrthreadmod.send_asr_thread_started) {
            framework_info_stru.asr_proc_obj->DeInit();
            asrthreadmod.send_asr_thread_started = false;
        }
        delete framework_info_stru.asr_proc_obj;
    }

    ttsthreadmod.tts_thread_stop = true;

    if (NULL != framework_info_stru.tts_proc_obj) {
        if (ttsthreadmod.tts_thread_start) {
            framework_info_stru.tts_proc_obj->DeInit();
            ttsthreadmod.tts_thread_start = false;
        }
        delete framework_info_stru.tts_proc_obj;
    }

    clockvoicethreadmod.clock_voice_thread_stop = true;

    if (NULL != framework_info_stru.clock_voice_obj) {
        if (clockvoicethreadmod.clock_voice_thread_start) {
            framework_info_stru.clock_voice_obj->DeInit();
            clockvoicethreadmod.clock_voice_thread_start = false;
        }
        delete framework_info_stru.clock_voice_obj;
    }

    volumecontrolthreadmod.volume_control_thread_stop = true;

    if (NULL != framework_info_stru.ctl_vol_obj) {
        if (volumecontrolthreadmod.volume_control_thread_start) {
            framework_info_stru.ctl_vol_obj->DeInit();
            volumecontrolthreadmod.volume_control_thread_start = false;
        }
        delete framework_info_stru.ctl_vol_obj;
    }

    return 0;
}
