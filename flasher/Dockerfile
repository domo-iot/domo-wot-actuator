FROM ubuntu:20.04

RUN apt-get -y update && \
    apt-get -y install python3 python3-pip curl
    
RUN python3 -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py)"

ENV PATH=$PATH:/root/.platformio/penv/bin

# add scripts for flashing shellys
RUN mkdir /work_dir

# script that generates shelly cert and key as well as basich auth credentials

ADD generateSecMaterial.sh /work_dir/

RUN chmod +x /work_dir/generateSecMaterial.sh

ADD ca/* /work_dir/

# stm32 firmware for shellydimmer    

ADD stm32.bin /work_dir/

# script that flashes firmware

ADD flash_device.py /work_dir/

ADD launchFlasher.sh /work_dir/

ADD requirements.txt /work_dir/

# install platformio config file

ADD platformio.ini /work_dir

RUN mkdir /work_dir/boards/

ADD boards/shelly1pm.json /work_dir/boards/

ADD boards/shellydimmer.json /work_dir/boards/

RUN cd /work_dir/ && \
    pio init && \
    pio pkg install

# lines 263-264 need to be commented out, otherwise an error "LD script not found" occurs if pio is launched with -t nobuild    
ADD main.py /root/.platformio/platforms/espressif8266/builder/    

#changed TIMEOUT_FOR_MB from 8 to 60, otherwise errors occur
ADD esptool.py /root/.platformio/packages/tool-esptoolpy/

# create bin dir inside pio folder

RUN mkdir -p /work_dir/.pio/build/shelly1

RUN mkdir -p /work_dir/.pio/build/shelly1pm

RUN mkdir -p /work_dir/.pio/build/shelly25

RUN mkdir -p /work_dir/.pio/build/shellydimmer

RUN mkdir -p /work_dir/.pio/build/shellyem

RUN mkdir -p /work_dir/.pio/build/shellyrgbw

RUN mkdir -p /work_dir/.pio/build/shelly1plus

RUN mkdir -p /work_dir/.pio/build/shelly1pmplus

RUN mkdir -p /work_dir/.pio/build/shelly2pmplus

RUN mkdir -p /work_dir/.pio/build/shelly2pmplussolo

# create data dir inside pio folder
RUN mkdir -p /work_dir/data/ /fw_binaries

ADD fw_binaries/* /fw_binaries/

WORKDIR /work_dir

ENTRYPOINT ["./launchFlasher.sh"]
