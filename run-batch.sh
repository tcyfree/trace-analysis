#!/bin/bash

runList="runList-msrc.txt" #runList存放准备运行的trace合集

function runTrace() {
  cat $1 | while read line
  do
    ./ssd 1  ../trace2/msrc/$line   
  done 
}
runTrace $runList
