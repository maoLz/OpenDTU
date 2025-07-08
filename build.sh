export https_proxy=http://127.0.0.1:7890 http_proxy=http://127.0.0.1:7890 all_proxy=socks5://127.0.0.1:7890

platformio run -e generic_esp32

 cp .pio/build/generic_esp32/firmware.bin ./
