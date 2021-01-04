#!/bin/bash

echo "运行self_start.sh"
chmod 777 ./serv_log_proc_start.sh
(sh ./serv_log_proc_start.sh 2>&1 | tee -a ./serv_log_proc_log.txt &)
sleep 1
(sh ./framework_start.sh 2>&1 | tee -a ./log.txt &)

