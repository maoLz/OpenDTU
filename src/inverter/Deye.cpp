#include "CommonUtils.h"
#include "Configuration.h"
#include "DeyeInverter.h"
#include "XmException.h"
#include "XmSmartStrategy.h"
#include "esp_task_wdt.h"
#include <AsyncJson.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>

// 构造函数
DeyeInverter::DeyeInverter()
{
    _index = 9;
    // 初始化代码
}

DeyeInverter::DeyeInverter(XM_INVERTER_T& inverter)
{
    _deviceSn = String(inverter.DeviceSn);
    _ip = IPAddress(inverter.Ip).toString();
    _maxPower = inverter.MaxPower;
    _ratedPower = inverter.RatedPower;
    _isOpen = inverter.Open;
    _type = inverter.Type;
    _index = inverter.Index;
    udp.begin(10000 + inverter.Index);
}

// 初始化方法
void DeyeInverter::init(AsyncWebServerRequest* request, CONFIG_T& config, JsonVariant& doc)
{
    // 初始化代码
    doc["code"] = 0;
    try {
        String spaceInverterSn = request->getParam("spaceInverterSn", true)->value();
        Configuration.clearInverterSlotBySpaceDeviceSn(spaceInverterSn);
        XM_INVERTER_T* inverter = Configuration.getFreeXMInverterSlot();
        if (inverter == nullptr) {
            // TODO 逆变器没有空位置

            throw CustomException("A maximum of two inverters can be added", INVERTER_LIMIT_TWO);
        }
        /**
         * 打开本地模式时，需要配置IP地址。调用APS接口
         */
        if (!request->hasParam("ip", true)) {
            throw CustomException("ip param not found.", PARAMS_NOT_FOUND);
        }
        String ip = request->getParam("ip", true)->value();
        IPAddress ipaddress;
        if (!ipaddress.fromString(ip)) {
            throw CustomException("ip format wrong.", PARAMS_NOT_FOUND);
        }
        /**
         * 将数据写入对象中
         */
        _ip = ip;
        _isOpen = true;
        _type = INVERTER_TYPE_DEYE;
        udp.begin(10000 + inverter->Index);
        JsonDocument response;
        int attemptTimes = 0;
        while (attemptTimes < 3 && _deviceSn.length() == 0) {
            if (attemptTimes == 0) {
                sendCommand("AT+Q\n", false);
            }
            sendCommand("", true);
            String str = syncRead(300, 5);
            str.replace("\020", "");
            handleResult(str, DEVICESN);
            attemptTimes++;
            esp_task_wdt_reset();
        }
        if (Configuration.getXMInverterByDeviceSn(_deviceSn.c_str()) != nullptr) {
            throw CustomException("Device already exists.", INVERTER_ALREADY_EXISTS);
        }
        attemptTimes = 0;
        while (attemptTimes < 5 && _ratedPower <= 0) {
            attemptTimes++;
            sendCommand("AT+INVDATA=8,01030010000185CF\n", false);
            String str = syncRead(200, 3);
            str.replace("\020", "");
            handleResult(str, RATEDPOWER);
            esp_task_wdt_reset();
        }
        attemptTimes = 0;



        if (_deviceSn.length() == 0 || _ratedPower <= 0) {
            throw CustomException("add deye inverter wrong.", INVERTER_DEYE_ADD_ERROR);
        }

        /**
         * 将数据写入配置中
         */

        inverter->Ip[0] = ipaddress[0];
        inverter->Ip[1] = ipaddress[1];
        inverter->Ip[2] = ipaddress[2];
        inverter->Ip[3] = ipaddress[3];
        inverter->RatedPower = _ratedPower;
        inverter->Open = open;
        inverter->Type = INVERTER_TYPE_DEYE;
        strlcpy(inverter->DeviceSn, _deviceSn.c_str(), DEVICE_SN_LENGTH);
        /**
         * 将当前逆变器添加至内存
         */
        SmartStrategy.addInverter(this, inverter->Index);
        _index = inverter->Index;
        /**
         * 返回数据
         */
        doc["deviceSn"] = _deviceSn;
        if (!Configuration.write()) {
            throw CustomException("dtu config write wrong.", DTU_CONF_WRITE_ERROR);
        }
        udp.stop();
    } catch (CustomException& e) {
        /**
         * 失败时，需要重置配置以及从配置还原内存中的属性
         */
        Configuration.read();
        loadByConfig();
        throw e;
    } catch (...) {
        Configuration.read();
        loadByConfig();
        throw CustomException("server fail");
    }
}

void DeyeInverter::remove(AsyncWebServerRequest* request, CONFIG_T& config, JsonVariant& doc)
{
    udp.stop();
}

// 根据配置加载
void DeyeInverter::loadByConfig()
{
    // 加载配置代码
}

// 检查是否开启
bool DeyeInverter::isOpen()
{
    // 返回是否开启的状态
    return _isOpen;
}

// 获取设备序列号
String DeyeInverter::getDeviceSn()
{
    // 返回设备序列号
    return _deviceSn;
}

void DeyeInverter::setInvertPower(int power)
{
    setInvertPower(power, false);
}

// 设置逆变器功率
void DeyeInverter::setInvertPower(int power, bool isWebRequest)
{

    if (_maxPower == 0) {
        return;
    }
    // TODO 临时兼容2000W接半路测试
    if (power < 0) {
        power = 0;
    } else if (power > _maxPower) {
        power = _maxPower;
    }
    int percent = power * 10000 / _ratedPower;


    if (power == _outputPower) {
        // if (!isWebRequest) {
        //     throw CustomException("无需设置指令");
        // }
        // return;
    }
    char powerPercent[5]; // 4 + null
    snprintf(powerPercent, sizeof(powerPercent), "%04X", percent);
    char commandPrefix[32];
    snprintf(commandPrefix, sizeof(commandPrefix), "01100035000102%s", powerPercent);
    char fullCommand[40];
    CommonUtils::calculateCRC16V2(commandPrefix, fullCommand, sizeof(fullCommand));
    char command[64];
    snprintf(command, sizeof(command), "AT+INVDATA=11,%s\n", fullCommand);

    int i = 0;
    char log[256];
    while (i < 3) {
        snprintf(log,sizeof(log),"[%s] %d time send command : %s.",_deviceSn,i+1,command);
        MessageOutput.println(log);
        sendCommandV2(command, isWebRequest && i == 0);
        esp_task_wdt_reset();
        //读一次响应
        syncRead(300,3);
        i++;
        if (!isWebRequest) {
            read53Power(3, power);
            int reduce = power - _outputPower;
            if (reduce < 5 && reduce > -5) {
                break;
            }
        }
    }
    snprintf(log,sizeof(log),"[%s]the real time 53 register value(maxOutputPower):%d",_deviceSn,_outputPower);
    MessageOutput.println(log);
    if (!isWebRequest) {
        int reduce = power - _outputPower;
        if (reduce > 5 || reduce < -5) {
            MessageOutput.println("set inverter command error.");
            throw CustomException("本次指令设置失败");
        }
    }
}

int DeyeInverter::getRatedPower()
{
    return _ratedPower;
}

// 获取逆变器功率
int DeyeInverter::getInvertPower()
{
    int i = 0;
    udp.begin(10000 + _index);
    int lastActivePower = _activePower;
    _activePower = -1;
    while (i < 4) {
        sendCommand(String("AT+INVDATA=8,010300560002241B\n"), i == 0);
        esp_task_wdt_reset();
        String result = syncRead(200, 5);
        result.replace(String("\020"), String(""));
        if (result.indexOf(String("+ok")) >= 0 && result.indexOf(String("0103")) == 4 && result.length() == 26) {
            udp.stop();
            result = result.substring(4);

            String number = result.substring(6, 10);
            int hexInt = CommonUtils::convertHexStrToInt(number);

            _activePower = hexInt / 10;
            _lastUpdateTime = millis();
            break;
        }
        i++;
    }
    udp.stop();
    if (_activePower == -1) {
        if (lastActivePower > 0 && (millis() - _lastUpdateTime < 5 * 60 * 1000)) {
            _activePower = lastActivePower;
            return _activePower;
        }
        throw CustomException("未读取逆变器功率");
    }
    // 返回逆变器功率
    return _activePower;
}

// 获取最大功率
int DeyeInverter::getMaxPower()
{

    return _maxPower;
}

int DeyeInverter::getCommandPower()
{
    return getOutputPower();
}

// 获取输出功率
float DeyeInverter::getOutputPower()
{
    int i = 0;
    while (_outputPower < 0 && i < 3) {
        read53Power(3, -1);
        i++;

    }
    return _outputPower;
}

void DeyeInverter::openSmartStrategy(bool open)
{
    if (open) {
        int lastPower = _maxPower;
        int i = 0;
        _maxPower = 0;
        udp.begin(10000 + _index);
        while (i < 5 && _maxPower <= 0) {
            sendCommand("AT+INVDATA=8,0103002800010402\n", i == 0);
            delay(1000);
            String str = syncRead(300, 5);
            str.replace("\020", "");
            handleResult(str, MAXPOWER);
            i++;
        }
        if (_maxPower > 0) {
            XM_INVERTER_T* inverter = Configuration.getXMInverterByDeviceSn(_deviceSn.c_str());
            if (inverter != nullptr) {
                inverter->MaxPower = _maxPower;
            }
            SmartStrategy.maxPower = SmartStrategy.maxPower - lastPower + _maxPower;
            if (!Configuration.write()) {
                throw CustomException("dtu config write wrong.", DTU_CONF_WRITE_ERROR);
            }
        } else {
            throw CustomException("deye get 40 power error.", UNEXPECT_ERROR);
        }
        udp.stop();
    }
}

void DeyeInverter::read53Power(int times, int targetPower)
{
    // 设置负数目的是在发送指令时不直接进入查询模式(i%5 != 0),避免重复进入查询模式
    int i = 0;
    int tempOutputPower = -1;
    while (i < times) {
        esp_task_wdt_reset();
        if (targetPower >= 0) {
            if (tempOutputPower == targetPower) {
                _outputPower = tempOutputPower;
                break;
            }
        } else {
            //没有目标功率时，只要读取实际值，就设置
            if (tempOutputPower >= 0) {
                _outputPower = tempOutputPower;
                break;
            }
        }
        sendCommandV2("AT+INVDATA=8,0103003500019404\n", targetPower == -1);
        esp_task_wdt_reset();
        String result = syncRead(300, 3);
        result.replace(String("\020"), String(""));
        if (result.indexOf(String("+ok")) >= 0) {
            if (result.indexOf(String("0110")) == 4) {
                // 设置寄存器指令响应结果开头,不处理
                continue;
            } else if (result.indexOf(String("0103")) == 4) {
                if (result.length() > 22) {
                    // 只针对指定长度响应进行处理
                    continue;
                }
                result = result.substring(4);
                String number = result.substring(6, 10);
                int hexInt = CommonUtils::convertHexStrToInt(number);
                tempOutputPower = hexInt / 10000.00 * _ratedPower;
                _outputPower = tempOutputPower;
                int reduce = targetPower - _outputPower;
                if (reduce < 5 && reduce > -5) {
                    // 有时会有浮点值，匹配在一定功率内就认为设置成功
                    break;
                }
            }
        }
        i++;
    }
}

// 获取类型
int DeyeInverter::getType()
{
    // 返回类型
    return _type;
}

int DeyeInverter::getIndex()
{
    return _index;
}

String DeyeInverter::getRemark()
{
    return String("this is deye inverter readUDP port:") + String(udp.serverPort()) + String(";setCommandUDP port:") + String(setCommandUdp.serverPort());
}

void DeyeInverter::sendExecuteCommand(String command, bool enterReadMode)
{
    // 发送命令的代码
    if (readErrorTimes > 10) {

        enterReadMode = true;
    }
    if (enterReadMode) {
        udp.beginPacket(_ip.c_str(), 48899);
        String readCommand = "WIFIKIT-214028-READ";
        //
        udp.write(CommonUtils::stringToUint8_tArray(readCommand), readCommand.length());
        udp.endPacket();
        String okCommand = "+ok";
        udp.beginPacket(_ip.c_str(), 48899);
        //
        udp.write(CommonUtils::stringToUint8_tArray(okCommand), okCommand.length());
        udp.endPacket();

    }
    //
    if (command.length() > 0) {
        udp.beginPacket(_ip.c_str(), 48899);

        udp.write(CommonUtils::stringToUint8_tArray(command), command.length());
        udp.endPacket();
    }
}

// 发送命令
void DeyeInverter::sendCommand(String command, bool enterReadMode)
{
    // 发送命令的代码
    if (readErrorTimes > 10) {
        char log[96];
        snprintf(log,sizeof(log),"[%s]read error times is too more ,prepare enter search mode.",_deviceSn);
        MessageOutput.println(log);
        enterReadMode = true;
        readErrorTimes = 0;
    }
    if (enterReadMode) {
        udp.beginPacket(_ip.c_str(), 48899);
        String readCommand = "WIFIKIT-214028-READ";
        //
        udp.write(CommonUtils::stringToUint8_tArray(readCommand), readCommand.length());
        udp.endPacket();
        String okCommand = "+ok";
        udp.beginPacket(_ip.c_str(), 48899);
        //
        udp.write(CommonUtils::stringToUint8_tArray(okCommand), okCommand.length());
        udp.endPacket();

    }
    //
    if (command.length() > 0) {
        udp.beginPacket(_ip.c_str(), 48899);

        udp.write(CommonUtils::stringToUint8_tArray(command), command.length());
        udp.endPacket();
    }
}

void DeyeInverter::sendCommandV2(const char* command, bool enterReadMode)
{
    if (readErrorTimes > 10) {
        char log[96];
        snprintf(log,sizeof(log),"[%s]read error times is too more ,prepare enter search mode.",_deviceSn);
        MessageOutput.println(log);
        enterReadMode = true;
        readErrorTimes = 0;
    }
    if (enterReadMode) {
        const char* readCommand = "WIFIKIT-214028-READ";
        udp.beginPacket(_ip.c_str(), 48899);
        udp.write(reinterpret_cast<const uint8_t*>(readCommand), strlen(readCommand));
        udp.endPacket();
        const char* okCommand = "+ok";
        udp.beginPacket(_ip.c_str(), 48899);
        udp.write(reinterpret_cast<const uint8_t*>(okCommand), strlen(okCommand));
        udp.endPacket();
    }
    if (command && command[0] != '\0') {
        udp.beginPacket(_ip.c_str(), 48899);
        udp.write(reinterpret_cast<const uint8_t*>(command), strlen(command));
        udp.endPacket();
    }
}

// 发送命令
void DeyeInverter::sendActiveCommand(String command, bool enterReadMode)
{
    if (readActiveEmptyTimes > 10) {

        enterReadMode = true;
    }
    if (enterReadMode) {
        delay(100);
        activePowerUdp.beginPacket(_ip.c_str(), 48899);
        String readCommand = "WIFIKIT-214028-READ";
        //
        activePowerUdp.write(CommonUtils::stringToUint8_tArray(readCommand), readCommand.length());
        activePowerUdp.endPacket();
        delay(100);
        String okCommand = "+ok";
        activePowerUdp.beginPacket(_ip.c_str(), 48899);
        //
        activePowerUdp.write(CommonUtils::stringToUint8_tArray(okCommand), okCommand.length());
        activePowerUdp.endPacket();

    }
    //
    if (command.length() > 0) {
        activePowerUdp.beginPacket(_ip.c_str(), 48899);

        activePowerUdp.write(CommonUtils::stringToUint8_tArray(command), command.length());
        activePowerUdp.endPacket();
        delay(100);
    }
}

String DeyeInverter::syncReadActive(int delayMillisecond, int cycleTimes)
{
    for (int i = 0; i < cycleTimes; i++) {
        delay(300);
        esp_task_wdt_reset();
        int readLength = activePowerUdp.parsePacket();
        //
        if (readLength > 0) {
            char buffer[1024];
            activePowerUdp.read(buffer, readLength);
            String ret = String((char*)buffer).substring(0, readLength);

            readActiveEmptyTimes = 0;
            return ret;
        } else {
            readActiveEmptyTimes++;
        }
    }
    return "";
}

String DeyeInverter::syncRead(int delayMillisecond, int cycleTimes)
{
    esp_task_wdt_reset();
    unsigned long start = millis();
    int i = 0;
    char buffer[64]; // 使用栈分配而非堆分配
    char logMsg[128];
    while (millis() - start < 500) { // 最多等待 300ms
        i++;
        int readLength = udp.parsePacket();
        //
        if (readLength > 0) {
            int bytesToRead = (readLength < sizeof(buffer)) ? readLength : sizeof(buffer)-1;
            udp.read(buffer, readLength);
            buffer[bytesToRead] = '\0';
            snprintf(logMsg, sizeof(logMsg), "[%s]revice data from udp socket:%s",_deviceSn.c_str(), buffer);
            MessageOutput.println(logMsg);
            readEmptyTimes = 0;
            return String(buffer).substring(0, bytesToRead);
        } else {
            readEmptyTimes++;
        }
        delay(5); // 防止 CPU 空转
    }
    snprintf(logMsg,sizeof(logMsg),"[WRONG][%s]read empty data from udp socket.",_deviceSn);
    MessageOutput.println(logMsg);
    readErrorTimes++;
    return "";
}

void DeyeInverter::handleResult(String result, int type)
{

    if (result.indexOf(_ip) >= 0 && type == DEVICESN) {
        _deviceSn = result.substring(result.lastIndexOf(",") + 1);
    } else if (result.indexOf("+ok") >= 0 && result.indexOf("0103") == 4) {
        result = result.substring(4);

        String number = result.substring(6, 10);

        int hexInt = CommonUtils::convertHexStrToInt(number);

        if (type == MAXPOWER && hexInt <= 100 && hexInt >= 50) {
            _maxPower = hexInt * _ratedPower / 100;
        } else if (type == OUTPUTPOWER) {
            _outputPower = (float)hexInt / 10000.00 * _ratedPower;
        } else if (type == ACTIVEPOWER) {
            _activePower = hexInt / 10;
        } else if (type == RATEDPOWER) {
            _ratedPower = hexInt / 10;
        }
    }
}
