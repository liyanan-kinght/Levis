<a id="top"></a>

# LED Python Class Description

* [Return to the top](description_api.md#top)

* description:     
  c call python.   
  Modify the pixel_ring code of the python driver led lights on the official website of Raspberry.  

* class file:     
  frame_work/respeaker_4mic_array_class.py  
  frame_work/frame_work.cpp  
  pixel_ring/pixel_ring/__init__.py  
  pixel_ring/pixel_ring/my_led_ring.py  
  pixel_ring/pixel_ring/my_pattern.py   

* API:  
```

 class cled_test(object):  

     def __init__(self):    
         #self.power = LED(5)  
         #pixel_ring.set_brightness(10)  
         pass
        
     def power_on_test(self):  
         self.power = LED(5)  
         self.power.on()  
         pixel_ring.set_brightness(10)  
    
     def power_off_test(self):  
         self.power.off()  

     def Led_lighton(self,dir):  
         pixel_ring.wakeup(dir)  
  
     def Led_lightoff(self):  
 	    pixel_ring.off()   

 void ledpower_init()  
 {  
     PyObject_CallMethod(pInstance,(char *)"power_on_test",(char *)"");     
     printf("ledpower_init.\n");    
 }  

 void ledpower_off()  
 {  
     PyObject_CallMethod(pInstance,(char *)"power_off_test",(char *)"");   
     printf("ledpower_off.\n");   
 }   

 void ledon_dir(int dir)   
 {   
     PyObject_CallMethod(pInstance,(char *)"Led_lighton",(char *)"i",dir);   
 }  

 void ledoff()   
 {   
     PyObject_CallMethod(pInstance,(char *)"Led_lightoff",(char *)"");   
 }   

```

