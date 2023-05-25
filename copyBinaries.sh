#!/bin/bash

cp .pio/build/shelly1/firmware.bin flasher/fw_binaries/shelly1.bin
cp .pio/build/shelly1pm/firmware.bin flasher/fw_binaries/shelly1pm.bin
cp .pio/build/shelly25/firmware.bin flasher/fw_binaries/shelly25.bin
cp .pio/build/shellydimmer/firmware.bin flasher/fw_binaries/shellydimmer.bin
cp .pio/build/shellyem/firmware.bin flasher/fw_binaries/shellyem.bin
cp .pio/build/shellyrgbw/firmware.bin flasher/fw_binaries/shellyrgbw.bin
cp .pio/build/shelly1plus/firmware.bin flasher/fw_binaries/shelly1plus.bin
cp .pio/build/shelly1plus/partitions.bin flasher/fw_binaries/partitions_shelly1plus.bin
cp .pio/build/shelly1pmplus/firmware.bin flasher/fw_binaries/shelly1pmplus.bin
cp .pio/build/shelly1pmplus/partitions.bin flasher/fw_binaries/partitions_shelly1pmplus.bin
cp .pio/build/shelly2pmplus/firmware.bin flasher/fw_binaries/shelly2pmplus.bin
cp .pio/build/shelly2pmplus/partitions.bin flasher/fw_binaries/partitions_shelly2pmplus.bin

cp .pio/build/shelly2pmplussolo/firmware.bin flasher/fw_binaries/shelly2pmplussolo.bin
cp .pio/build/shelly2pmplussolo/partitions.bin flasher/fw_binaries/partitions_shelly2pmplussolo.bin



cp platformio.ini flasher/
