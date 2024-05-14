#!/bin/bash
if [ $# -ge 2 ]
then
   if [ -e $1 ]
   then
       echo $2 > $1
       echo "The file $1 overwrite with $2"
   else
       mkdir -p $(dirname $1)
       echo $2 > $1
       echo "The file $1 overwrite with $2"
   fi
else
   echo "Argument counts are less than expected"
   exit 1
fi
if [ $? -ne 0 ]
then
    echo "Failed to create file and write to it"
    exit 1
else
    echo "File Write successful"
fi
