<a id="top"></a>

# Play Class Description

* [Return to the top](description_api.md#top)

* class file:     
  frame_work/play_audio.cpp    
  frame_work/play_audio.h     

* API:    
```

 enum PLAY_ACTION  
 {  
     PLAY_MUSIC,  
     STOP_MUSIC,  
     PLAY_AWAKE,  
     PLAY_NO_HEAR,  
     PLAY_NET_ERR,  
     PLAY_TTS,  
     PLAY_NOT_UNDERSTAND,  
     PLAY_CLOCK_TTS,  
     PLAY_CLOCK_TTS_FILE,  
     PLAY_CLOCK_RES_OK,  
     PLAY_DEFAULT  
 };

 class Play_Audio_Data   
 {  
 public:  
     Play_Audio_Data();   
     ~Play_Audio_Data();   

     int Init();   
     int DeInit();   

     int pause();    
     int SendSig(PLAY_ACTION play_cation); //Play the corresponding sound according to the playback type   
     int set_tts_voice_info(char *data ,int len); //Incoming TTS converted sound      
     int set_clock_tts_voice_info(char *data ,int len); //Incoming TTS converted clock voice      
     int set_music_info(char *song,int song_len,char *artist,int artist_len);   

     CURLcode App_Binding_Token(const std::string & strUrl, int nTimeout, const char *token_nm); //Bind Kugou Music Account    

 }; 
   
```
