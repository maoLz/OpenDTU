// #include "XmUdp.h"
// #include "MessageOutput.h"
// #include <cstdint> // For uint8_t and uint16_t
// #include "XmShelly.h"
// #include "XmMessage.h"

// /**
//     由于udp 发送接受为异步，将读取输出功率和设置功率放在不同udp
//  */

// XmUdpClass::XmUdpClass()
//     : _loopTask(10 * TASK_SECOND, TASK_FOREVER, std::bind(&XmUdpClass::loop, this)),
//       _readTask(10 * TASK_SECOND, TASK_FOREVER, std::bind(&XmUdpClass::readLoop, this))
// {
// }

// void XmUdpClass::init(AsyncWebServer& server, Scheduler& scheduler)
// {
//     using std::placeholders::_1;
//     scheduler.addTask(_loopTask);
//     _loopTask.setInterval(10 * TASK_SECOND);
//     _loopTask.enable();
//     int beginResult = readUdp.begin(10000);
//     int beginResult2 = writeUdp.begin(10002);

// }

// void XmUdpClass::loop()
// {
//     if(!(Shelly.open)){
//         _loopTask.forceNextIteration();
//         return;
//     }
//     sendCommand("AT+INVDATA=8,0103003500019404\n",true);
//     MessageOutput.print("Shelly open");
//     MessageOutput.println(Shelly.open);
//     if(!Shelly.open){
//         _loopTask.forceNextIteration();
//         MessageOutput.println("not loop");
//         return;
//     }

//     if(outputPower >= 0){
//         try{
//             MessageOutput.println("udp getTotalPower");
//             int totalPower = Shelly.getTotalPower();
//             int needSetPower = smartStrategyFunc(totalPower,outputPower);
//             if(needSetPower != outputPower){
//                 int percent = needSetPower * 12.5;
//                 MessageOutput.print("need set invert power:");
//                 MessageOutput.println(needSetPower);
//                 setInvertPower(percent);
//             }
//             sendCommand("AT+INVDATA=8,0103003500019404\n",false);
//         }catch(CustomException& e){
//             MessageOutput.println(e.msg());
//         }
//     }else{
//         sendCommand("AT+INVDATA=8,0103003500019404\n",true);
//     }
// void XmUdpClass::sendCommand(WiFiUDP udp,String command){
//     udp.beginPacket("192.168.3.64", 48899);
//     XmMessage.printStr(command);
//     MessageOutput.println(command);
//     udp.write(stringToUint8_tArray(command), command.length());
//     udp.endPacket();
// }

// /**
//  * WIFIUDP未提供阻塞读方法，手动实现
//  * @param delayMillisecond 读取一次的超时时间
//  * @param cycleTimes 读取的次数
//  */
// String XmUdpClass::syncRead(WiFiUDP udp,int delayMillisecond,int cycleTimes){
//     for(int i=0;i<cycleTimes;i++){
//         int readLength = udp.parsePacket();
//         MessageOutput.print("parsePacket: ");
//         MessageOutput.println(readLength);
//         if(readLength > 0){
//             char buffer[1024];
//             String str = String("parsePacket: ") + String(readLength);
//             XmMessage.printStr(str);
//             udp.read(buffer,readLength);
//             String ret= String((char*)buffer).substring(0, readLength);
//             XmMessage.printStr(ret);
//             return ret;
//         }else{
//             delay(delayMillisecond);
//         }
//     }
//     return "";
// }

// void XmUdpClass::loop(){
//     f(errorTimes > 3){
//             errorTimes = 0;
//             openReadMode(readUdp);
//      }
//      sendCommand(readUdp,"AT+INVDATA=8,010300560001641A\n");

// }

// void XmUdpClass::read(){
//
// }

// int XmUdpClass::smartStrategyFunc(int totalPower,int invertPower){
//     //当前空间功率 + 微逆最大设置功率
//     int modifyPower = totalPower + invertPower;
//     if(modifyPower < 0){
//         modifyPower = 0;
//     }else if(modifyPower > 800){
//         modifyPower = 800;
//     }
//     return modifyPower;
// }

// void XmUdpClass::readLoop(){
//
//     }
// }


// /*
//     getOutputPower
// */
// int XmUdpClass::handleRead(String ret){
//     try{
//         //sendCommand("AT+INVDATA=8,010300560001641A\n");
//         ret = ret.substring(4);
//         ret.replace("", "");
//         MessageOutput.print("ret:");
//         MessageOutput.println(ret);
//         String outPower = ret.substring(6,10);
//         MessageOutput.print("outPower:");
//         MessageOutput.println(outPower);
//         int outPowerInt = convertHexStrToInt(outPower);
//         MessageOutput.print("outPowerInt:");
//         MessageOutput.println(outPowerInt);
//         return outPowerInt/10;
//     }catch(CustomException& e){
//         return -1;
//     }
// }

// void XmUdpClass::setInvertPower(int powerPercentNumber){
//     try{
//        String powerPercent = addZeroPrefix(String(powerPercentNumber,HEX),4);
//         MessageOutput.print("powerPercent:");
//         MessageOutput.println(powerPercent);
//         String commandPrefix = String("01100035000102") + powerPercent;
//         String command = "AT+INVDATA=11," + calculateCRC16(commandPrefix);
//         MessageOutput.print("command:");
//         MessageOutput.println(command);
//         sendCommand(command + "\n");
//         MessageOutput.print("str:");
//         MessageOutput.println(str);
//     }catch(CustomException& e){
//     }
// }


// int XmUdpClass::convertHexStrToInt(String hex){
//     return (int)strtol(hex.c_str(), NULL, 16);
// }


// String XmUdpClass::initReadCommand(int registerAddr,int registerNumber){
//     String readPrefix = String("0103");
//     String ret = readPrefix + addZeroPrefix(String(registerAddr,HEX),4) + addZeroPrefix(String(registerNumber),4);
//     return calculateCRC16(ret);
// }


// String XmUdpClass::addZeroPrefix(String ret, int width)
// {
//     while (ret.length() < width)
//     {
//         ret = "0" + ret;
//     }
//     return ret;
// }

// String XmUdpClass::calculateCRC16(String commandPrefix) {
//     uint8_t commandIntArray[commandPrefix.length()/2];
//     for(int i=0;i<commandPrefix.length();i+=2){
//         String hex = commandPrefix.substring(i,i+2);
//         int num = strtoul(hex.c_str(),NULL,16);
//         commandIntArray[i/2] = num;
//     }
//     uint16_t crc = 0xFFFF; // Initial value
//     for (size_t i = 0; i < sizeof(commandIntArray); i++) {
//         crc ^= commandIntArray[i]; // XOR byte into least significant byte of crc
//         for (size_t j = 0; j < 8; j++) { // Loop over each bit
//             if (crc & 0x0001) { // If the LSB is set
//                 crc >>= 1; // Shift right
//                 crc ^= 0xA001; // XOR with the polynomial
//             } else {
//                 crc >>= 1; // Just shift right
//             }
//         }
//     }
//     String crcHex = String(crc, HEX);
//     crcHex.toUpperCase();
//     while(crcHex.length()<4){
//         crcHex = String("0") + crcHex;
//     }
//     return commandPrefix +crcHex.substring(2,4) + crcHex.substring(0,2);
// }


// uint8_t* XmUdpClass::stringToUint8_tArray(String str) {
//     int stringLength = str.length();
//     uint8_t* buffer = new uint8_t[stringLength];
//     for (int j = 0; j < stringLength; j++)
//     {
//         buffer[j] = str[j];
//     }
//     return buffer;
// }

// /**
//  * WIFIUDP未提供阻塞读方法，手动实现
//  * @param delayMillisecond 读取一次的超时时间
//  * @param cycleTimes 读取的次数
//  */
// String XmUdpClass::syncRead(int delayMillisecond,int cycleTimes){
//     for(int i=0;i<cycleTimes;i++){
//         int readLength = udp.parsePacket();
//         if(readLength > 0){
//             MessageOutput.println("read data length:" + String(readLength));
//             char buffer[1024];
//             udp.read(buffer,readLength);
//             String ret= String((char*)buffer).substring(0, readLength);
//             MessageOutput.println("read data:" + ret);
//             readEmptyTimes=0;
//             return ret;
//         }else{
//             readEmptyTimes++;
//             delay(delayMillisecond);
//         }
//     }
//     return "";
// }




// void XmUdpClass::sendCommand(String command,bool enterReadMode){
//     if(readEmptyTimes > 50){
//         MessageOutput.println("read data empty too many times." + String(readEmptyTimes));
//         enterReadMode = true;
//     }
//     if(enterReadMode ){
//         udp.beginPacket("192.168.3.64", 48899);
//         String readCommand = "WIFIKIT-214028-READ";
//         MessageOutput.println("send command:" + readCommand);
//         udp.write(stringToUint8_tArray(readCommand), readCommand.length());
//         udp.endPacket();
//         String okCommand = "+ok";
//         udp.beginPacket("192.168.3.64", 48899);
//         MessageOutput.println("send command:" + okCommand);
//         udp.write(stringToUint8_tArray(okCommand), okCommand.length());
//         udp.endPacket();
//     }
//     udp.beginPacket("192.168.3.64", 48899);
//     MessageOutput.println("send command:" + command);
//     udp.write(stringToUint8_tArray(command), command.length());
//     udp.endPacket();




// /**
//     deye 下发AT指令时，必须先开启读模式
//  */
// void XmUdpClass::openReadMode(WiFiUDP udp){
//         sendCommand(udp,"WIFIKIT-214028-READ");
//         sendCommand(udp,"+ok");
// }









// XmUdpClass XmUdp;
