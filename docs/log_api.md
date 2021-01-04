<a id="top"></a>

# Log Class Description

* [Return to the top](description_api.md#top)

*Log dump is a simple cs architecture.*   
*Use socket and epoll*  

## client log send:

* class api file: client_log_dump.cpp client_log_dump.h 

* test file : client_log_dump_lib/client_main_test.cpp   

* Send Log API :    
```

  SENDLOG_CLI_OBJ_PTR(sendlogobj,log_level_value,format,...) //clent send log class object pointer   
  SENDLOG_CLI_OBJ(sendlogobj,log_level_value,format,...)     //cleant send log class object  
      
```

## server log dump:

* Is a server program.
* class api file:    
  server_log_dump/server_log_dump.cpp     
  server_log_dump/server_log_dump.h    
