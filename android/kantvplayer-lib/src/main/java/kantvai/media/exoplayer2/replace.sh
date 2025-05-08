#!/usr/bin/env bash

 #zhouwg, replace all specify string in all .c/.cpp source files in the project 


find ./ -type f -name '*.java' -print | while read i
do
    #echo "$i"
    if [ $i != "./replace.sh" ]; then
 sed 's|com\.google\.android\.exoplayer2|kantvai\.media\.exoplayer2|g' $i > $i.tmp && mv $i.tmp $i && chmod 755 $i
# sed 's|CDELog|KANTVLog|g' $i > $i.tmp && mv $i.tmp $i && chmod 755 $i
# sed 's|cdeos\.media\.player|kantvai\.media\.player|g' $i > $i.tmp && mv $i.tmp $i && chmod 755 $i
 sed 's|CDEHttpsURLConnection|KANTVHttpsURLConnection|g' $i > $i.tmp && mv $i.tmp $i && chmod 755 $i
    fi
done
