#pragma once

#include <ESPAsyncWebServer.h>

class CommonUtils {
public:
    static bool isTrue(AsyncWebServerRequest* request,const char* key);

    static uint8_t* stringToUint8_tArray(String str);

    static int convertHexStrToInt(String hex);

    static String addZeroPrefix(String ret, int width);

    static String calculateCRC16(String commandPrefix);
};
