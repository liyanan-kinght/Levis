<a id="top"></a>

# Clock Class Description

* [Return to the top](description_api.md#top)

* class file:    
  frame_work/clock_proc.cpp   
  frame_work/clock_proc.cpp  

* API:
```

 class  ClockVoiceProc   
 {     
     public:

     CLOCK_VOICE_INFO clock_voice_stru;

     int Init();   
     int DeInit();   

     int AnalysisAsrResult(const  char * asr_result);   
     int send_clock_sig(); //Send a signal to the thread to get the voice   
     int time_count_proc(); //Alarm time count   
     int play_select(); //Determine whether the voice is obtained from the server   

     int set_clock_voice_info_init(); //Alarm processing status value   
     int set_clock_voice_info_deinit(); //Alarm processing status value    

 };  
  
```
