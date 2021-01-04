#!/bin/bash

echo "开始执行程序！！！！"
echo "Respeaker_Framework_Main"
export LD_LIBRARY_PATH=./:./libs:$LD_LIBRARY_PATH
ulimit -c unlimited 
chmod a+x Respeaker_Framework_Main
./Respeaker_Framework_Main 
