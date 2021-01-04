<a id="top"></a>

# TTS Class Description

* [Return to the top](description_api.md#top)

* class file:     
  frame_work/tts_proc.cpp    
  frame_work/tts_proc.cpp     

* API:   
```

 typedef void * (*TTS_RES)(const char* pcm_buf, int pcm_buf_len,void * arg); //callback

 class TtsProc  
 {  
 public:  
     TtsProc();  
     ~TtsProc();  
     int Init();  
     int DeInit();  
     int pause(); //Stop all operations that interact with the TTS service    
     int set_tts_info(const char *ttsTxt,int len,TTS_RES callback,void *arg); //Send text to get voice   
 }; 
   
```
