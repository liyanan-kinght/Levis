#!/bin/bash

set -xeuo pipefail

buildfolder='buildRPI'

if [ ! -d $buildfolder ];then
      mkdir -p $buildfolder
fi

cd $buildfolder
#rm -rf *
cmake -DARMV7_COMPILER=on -DOPENMP_FLAG=on -DFRAME_WORK_FLAG=on ..
make

#collect bin and lib into folder for pushing to board
cd ..
runonboard_folder='runonboard'

if [ ! -d $runonboard_folder ];then
      mkdir -p $runonboard_folder
fi

cp $buildfolder/Respeaker_Framework_Main $runonboard_folder/
cp $buildfolder/server_log_dump/Server_Log_Dump_App $runonboard_folder/
cp ./scripts/*  $runonboard_folder/
cp -d -rf libs $runonboard_folder/
#cp -d -rf local_music $runonboard_folder/
cp $buildfolder/client_log_dump_lib/*.so $runonboard_folder/libs/
cp ./tflite_model/converted_model.tflite $runonboard_folder/converted_model.tflite
cp ./local_pcm/*.pcm $runonboard_folder/
cp frame_work/respeaker_4mic_array.py $runonboard_folder/
cp frame_work/respeaker_4mic_array_class.py $runonboard_folder/
cp -d -rf pixel_ring/pixel_ring $runonboard_folder/
cp ./config_dir/lefar.cfg  $runonboard_folder/
echo "copy files into runonboard_folder"
