ultra-remote (ESP-IDF variant)
==============================

work in progress...

preferring arduino-esp32? have a look at this: [fast ESP32 WiFi remote control (arduino-esp32 variant)](https://github.com/sp4rkie/ultra-remote-ino)

what is it
----------

- a fast ESP32 WiFi remote control with buttons
- various sizes are supported (currently 1 - 16 buttons)

main purpose
------------

- remote control of any machine suitable to receive ascii commands (including smartphones)

basic functionality
-------------------

- wake up after key press
- connect to WiFi
- send an ascii command to a remote machine
- receive status of the remote machine
- go to deep sleep again

features
--------

- very fast
- time between key press (aka. wakeup) and status receive averages 220ms
- battery operated (CR123 A 3V)

build environment
-----------------

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html#)

how to build
------------

- setup ESP-IDF as documented [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#installation)
- then

        . ~/esp/esp-idf/export.sh
        git clone git@github.com:sp4rkie/ultra-remote-idf
        cd ultra-remote-idf
        idf.py flash monitor

example debug output
--------------------


        Entering deep sleep
        ets Jun  8 2016 00:22:57

        rst:0x5 (DEEPSLEEP_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
        configsip: 0, SPIWP:0xee
        clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
        mode:DIO, clock div:1
        load:0x3fff0030,len:1416
        load:0x40078000,len:14716
        ho 0 tail 12 room 4
        load:0x40080400,len:4
        load:0x40080404,len:3372
        entry 0x400805a4
        Enabling EXT1 wakeup on pins GPIO2
        E (43) TP: a
        E (83) TP: b
        E (83) TP: c
        E (83) TP: d
        E (83) TP: 1
        E (123) TP: Waiting for IP(s)
        E (163) TP: 2
        E (183) TP: 3
        E (193) TP: 4
        E (203) TP: XXXXXXXXXXXXXXXXXXX                     # <-- 203ms elapsed since keypress (wakeup)
        E (203) wifi:NAN WiFi stop
        Entering deep sleep

