#!/bin/bash

echo "开始执行程序！！！！"
echo "Server_Log_Dump_App"
export LD_LIBRARY_PATH=./:$LD_LIBRARY_PATH
ulimit -c unlimited 
chmod a+x ./Server_Log_Dump_App
./Server_Log_Dump_App 

