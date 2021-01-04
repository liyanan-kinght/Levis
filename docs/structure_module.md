<a id="top"></a>

# structure && module description

* [Return to the top](Readme.md#top)

directory structure:
---------------------------------

```
.    
├── art_picture   
│   └── smart_voice_speaker_log.png    
├── client_log_dump_lib    
│   ├── CMakeLists.txt    
│   └── src    
│       ├── client_log_dump.cpp    
│       ├── client_log_dump.h    
│       └── client_main_test.cpp   
├── CMakeLists.txt   
├── compile.sh   
├── config_dir   
│   ├── lefar.cfg   
│   └── rc.local   
├── docs   
│   ├── asr_api.md    
│   ├── clock_api.md   
│   ├── description_api.md    
│   ├── frame_work.md   
│   ├── led_api.md    
│   ├── log_api.md    
│   ├── play_api.md    
│   ├── Readme.md    
│   ├── release_note.md   
│   ├── structure_module.md   
│   └── tts_api.md   
├── frame_work    
│   ├── cJSON.cpp   
│   ├── cJSON.h   
│   ├── clock_proc.cpp   
│   ├── clock_proc.h   
│   ├── frame_work_common_def.h   
│   ├── frame_work.cpp   
│   ├── frame_work.h    
│   ├── frame_work_main.cpp   
│   ├── framework_volume_ctl.cpp   
│   ├── framework_volume_ctl.h   
│   ├── play_audio.cpp   
│   ├── play_audio.h   
│   ├── respeaker_4mic_array_class.py    
│   ├── respeaker_4mic_array.py   
│   ├── send_asr.cpp   
│   ├── send_asr.h   
│   ├── tts_proc.cpp   
│   └── tts_proc.h     
│   ├── test_env.cpp     
│   └── test_env.h       
├── include   
│   ├── curl   
│   ├── globaldef.h   
│   ├── libavutil    
│   ├── libswresample    
│   ├── mad.h   
│   └── preproc.h    
├── libs    
├── local_pcm       
├── pixel_ring    
├── README.md    
├── scripts    
│   ├── framework_start.sh   
│   ├── self_start.sh    
│   └── serv_log_proc_start.sh   
├── server_log_dump   
│   ├── CMakeLists.txt    
│   └── src   
│       ├── server_log_dump.cpp   
│       ├── server_log_dump.h   
│       └── server_main.cpp   
└── tflite_model   
    └── converted_model_10cnn_float_v1.16.tflite       

```

Project description of each module
----------------------------------

1. log record:  
   dir: client_log_dump_lib  &&  server_log_dump   
   describe:The logging module of the program.  

2. The module that plays the sound:  
   file:play_audio.cpp play_audio.h  
   describe:Play music and hints.  

3. ASR(Automatic Speech Recognition) interaction module:  
   file: send_asr.cpp send_asr.h  
   describe: Send a voice to the ASR server and get the result.  

4. TTS(Text To Speech) interaction modul:  
   file:tts_proc.cpp tts_proc.h  
   describe: Text To Speech proc.  

5. Control of Raspberry Pie MIC array LED lights:  
   file:respeaker_4mic_array_class.py  

6. Intelligent voice speaker logic control Test:  
   file:frame_work.cpp frame_work.h  
   describe:Intelligent voice speaker control logic.  
   
7. JSON format parsing:  
   file:cJSON.cpp cJSON.h  
   describe: Open source cJSON is used. 

8. Contril volume :     
   file:framework_volume_ctl.h framework_volume_ctl.cpp    
   describe: Control volume.    
