ultra-remote (ESP-IDF variant)
==============================

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

