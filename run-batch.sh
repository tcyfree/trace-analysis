#!/bin/bash

runList="runList.txt" #runList存放准备运行的trace合集

function runTrace() {
  cat $1 | while read line
  do
    ./ssd 1  ../trace/$line   
  done 
}
runTrace $runList
