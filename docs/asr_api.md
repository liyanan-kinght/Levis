<a id="top"></a>

# ASR Class Description

* [Return to the top](description_api.md#top)

* class file:    
  frame_work/send_asr.cpp  
  frame_work/send_asr.h  

* API:
```

 class InteractiveProcForASR  
 {  
     public:  
     int Init();  
     int DeInit();       
     int GetAsrResult(char * in); //Get the result returned by asr
     int set_send_asr_info(char * send_buf,int send_buf_len,bool vadfg); //Send voice data and vad identification to ASR    
     int pause();   //Stop all operations interacting with the ASR service   
 }; 
  
```
