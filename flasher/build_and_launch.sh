#!/bin/bash

file_array=(ca/ca-domo.crt ca/ca-domo.key fw_binaries/shelly1.bin fw_binaries/shelly1pm.bin fw_binaries/shelly25.bin fw_binaries/shellyem.bin fw_binaries/shellydimmer.bin fw_binaries/shellyrgbw.bin  fw_binaries/shelly1plus.bin fw_binaries/shelly1pmplus.bin fw_binaries/shelly2pmplus.bin)

for FILE in "${file_array[@]}"
do
    if ! test -f "$FILE"; then
        echo "$FILE does not exist"
        echo "Attention !!! Please remember to put ca-domo.crt and ca-domo.key in ca/ folder and binaries into fw_binaries folder"
        exit 1
    fi
done

docker build -t domoshellyflasher .
docker run -it --rm --privileged --device=/dev/ttyUSB0 --name domoshellyflasher -v $PWD/out:/out domoshellyflasher
