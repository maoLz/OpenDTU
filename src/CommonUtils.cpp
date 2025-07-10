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

void CommonUtils::calculateCRC16V2(const char* commandHex, char* outBuffer, size_t outBufferSize) {
    size_t len = strlen(commandHex);
    size_t byteLen = len / 2;
    if (byteLen > 64 || outBufferSize < len + 4 + 1) return; // 安全检查

    uint8_t data[64];
    for (size_t i = 0; i < byteLen; ++i) {
        char byteStr[3] = { commandHex[i * 2], commandHex[i * 2 + 1], 0 };
        data[i] = (uint8_t)strtoul(byteStr, nullptr, 16);
    }

    // CRC16计算（Modbus）
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < byteLen; i++) {
        crc ^= data[i];
        for (size_t j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }

    // 结果拼接（低字节在前，高字节在后）
    snprintf(outBuffer, outBufferSize, "%s%02X%02X", commandHex, crc & 0xFF, (crc >> 8) & 0xFF);
}
