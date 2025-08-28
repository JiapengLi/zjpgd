#!/bin/bash


CLI=./zjdtest
IMAGES=samples/*.jpg

for img in $IMAGES; do
    $CLI "$img"
    sleep 0.5
done

CLI=./zjdcli
IMAGES=samples/*.jpg

for img in $IMAGES; do
    $CLI "$img" > samples/$(basename "$img" .jpg).txt
done

CLI=./zjdcli_debug
IMAGES=samples/*.jpg

for img in $IMAGES; do
    $CLI "$img" > samples/$(basename "$img" .jpg)-debug.txt
done


