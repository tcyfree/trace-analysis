#!/bin/bash
rm -rf out-batch.txt

runList="runList.txt" #runList存放准备运行的trace合集

function runTrace() {
  #1.找到运行的excel
  cat $1 | while read line
  do
    echo $line
  done 

  cat $1 | while read line
  do
    saveFile1=${line##*/}
    saveFile2=${saveFile1%%.*}

    echo $saveFile2"," >> out-batch.txt
    ./ssd 1  ../trace/$line   
    cat trace-analysis.txt >> out-batch.txt

  done 
}
runTrace $runList
