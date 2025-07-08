// #pragma once

// #include "Configuration.h"
// #include <ESPAsyncWebServer.h>
// #include <TaskSchedulerDeclarations.h>
// #include "XmException.h"


// class XmUdpClass{

// public:
//     //仅
//     WiFiUDP readUdp;
//     WiFiUDP writeUdp;
//     XmUdpClass();
//     void init(AsyncWebServer& server, Scheduler& scheduler);
//     void sendCommand(WiFiUDP udp,String command);
//     String initReadCommand(int registerAddr,int registerNumber);
//     String calculateCRC16(String data);
//     String addZeroPrefix(String ret, int width);
//     uint8_t* stringToUint8_tArray(String str);
//     String syncRead(WiFiUDP udp,int delayMillisecond,int cycleTimes);
//     int handleRead(String ret);
//     int convertHexStrToInt(String hex);
//     void test(int setPower);
//     int smartStrategyFunc(int totalPower,int invertPower);
//     void setInvertPower(int percent);

//     int outputPower=-1;
//     int readEmptyTimes = 0;
//     bool noRead= false;

//     void openReadMode(WiFiUDP udp);
// private:
//     Task _loopTask;
//     Task _readTask;
//     void loop();
//     void readLoop();
//     uint8_t time=0;
//     uint8_t errorTimes=5;
//     int outputPower=0;
//     bool isReadMode=false;
// };

// extern XmUdpClass XmUdp;
