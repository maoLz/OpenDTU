#include "CommonUtils.h"

bool CommonUtils::isTrue(AsyncWebServerRequest* request,const char* key) {
    String str = request->getParam(String(key),true)->value();
    str.toLowerCase();
    return String("true").equals(str);
}

uint8_t* CommonUtils::stringToUint8_tArray(String str) {
    int stringLength = str.length();
    uint8_t* buffer = new uint8_t[stringLength];
    for (int j = 0; j < stringLength; j++)
    {
        buffer[j] = str[j];
    }
    return buffer;
}

int CommonUtils::convertHexStrToInt(String hex){
    return (int)strtol(hex.c_str(), NULL, 16);
}



String CommonUtils::addZeroPrefix(String ret, int width)
{
    while (ret.length() < width)
    {
        ret = "0" + ret;
    }
    return ret;
}

String CommonUtils::calculateCRC16(String commandPrefix) {
    uint8_t commandIntArray[commandPrefix.length()/2];
    for(int i=0;i<commandPrefix.length();i+=2){
        String hex = commandPrefix.substring(i,i+2);
        int num = strtoul(hex.c_str(),NULL,16);
        commandIntArray[i/2] = num;
    }
    uint16_t crc = 0xFFFF; // Initial value
    for (size_t i = 0; i < sizeof(commandIntArray); i++) {
        crc ^= commandIntArray[i]; // XOR byte into least significant byte of crc
        for (size_t j = 0; j < 8; j++) { // Loop over each bit
            if (crc & 0x0001) { // If the LSB is set
                crc >>= 1; // Shift right
                crc ^= 0xA001; // XOR with the polynomial
            } else {
                crc >>= 1; // Just shift right
            }
        }
    }
    String crcHex = String(crc, HEX);
    crcHex.toUpperCase();
    while(crcHex.length()<4){
        crcHex = String("0") + crcHex;
    }
    return commandPrefix +crcHex.substring(2,4) + crcHex.substring(0,2);
}
