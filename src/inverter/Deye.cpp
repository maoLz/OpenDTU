#include "CommonUtils.h"
#include "Configuration.h"
#include "DeyeInverter.h"
#include "MessageOutput.h"
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
            MessageOutput.println("DTU inverter is out of size.You need Remove one Inverter.");
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

        MessageOutput.println(String("ip地址:") + String(_ip) + String(",获取设备SN:") + _deviceSn);
        MessageOutput.println(String("ip地址:") + String(_ip) + String("获取额定功率:") + _ratedPower);
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
    MessageOutput.print(String("deviceSn:") + _deviceSn + String(",设置百分比:"));
    MessageOutput.println(percent);
    if (power == _outputPower) {
        // if (!isWebRequest) {
        //     throw CustomException("无需设置指令");
        // }
        // return;
    }
    // 设置功率的代码
    String powerPercent = CommonUtils::addZeroPrefix(String(percent, HEX), 4);
    String commandPrefix = String("01100035000102") + powerPercent;
    String command = "AT+INVDATA=11," + CommonUtils::calculateCRC16(commandPrefix);
    // MessageOutput.println(command);

    int i = 0;
    while (i < 3) {
        sendCommand(command + "\n", isWebRequest && i == 0);
        esp_task_wdt_reset();
        //读一次响应
        syncRead(300,3);
        i++;
        if (!isWebRequest) {
            read53Power(3, power);
            int reduce = power - _outputPower;
            if (reduce < 5 && reduce > -5) {
                // 有时会有浮点值，匹配在一定功率内就认为设置成功
                break;
            }
        }
    }
    if (!isWebRequest) {
        int reduce = power - _outputPower;
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",目标功率:") + String(power));
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",53号寄存器值:") + String(_outputPower));
        if (reduce > 5 || reduce < -5) {
            MessageOutput.println(String("deviceSn:") + _deviceSn + String(",目标功率与53号寄存器值不同,本次指令设置失败。"));
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
            MessageOutput.println(String("deviceSn:") + _deviceSn + String("逆变器当前输出功率读取为:") + String(hexInt));
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
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",首次读取输出功率值为") + _outputPower);
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
        sendCommand(String("AT+INVDATA=8,0103003500019404\n"), targetPower == -1);
        esp_task_wdt_reset();
        String result = syncRead(300, 3);
        result.replace(String("\020"), String(""));
        if (result.indexOf(String("+ok")) >= 0) {
            if (result.indexOf(String("0110")) == 4) {
                // 设置寄存器指令响应结果开头,不触发i++,避免重新进入查询模式，
                // if(targetPower >= 0){
                //     MessageOutput.println(String("读取到设置寄存器响应结果，而且目标功率有值则直接认为成功，最大输出功率为")+String(targetPower));
                //     //读取到设置寄存器响应结果，而且目标功率有值则直接认为成功
                //     _outputPower = targetPower;
                //     break;
                // }
                continue;
            } else if (result.indexOf(String("0103")) == 4) {
                if (result.length() > 22) {
                    continue;
                }
                // esp_task_wdt_reset();
                // activePowerUdp.stop();
                result = result.substring(4);
                String number = result.substring(6, 10);
                // MessageOutput.println(String("number:") + String(number));
                int hexInt = CommonUtils::convertHexStrToInt(number);
                // MessageOutput.println(String("hexInt:") + String(hexInt));
                //  MessageOutput.println("hexInt/10000.00:" + String(hexInt / 10000.00));
                tempOutputPower = hexInt / 10000.00 * _ratedPower;
                MessageOutput.println(String("deviceSn:") + _deviceSn + String(",53寄存器功率:") + String(tempOutputPower));
                MessageOutput.println(String("deviceSn:") + _deviceSn + String(",目标功率:") + String(targetPower));

                // MessageOutput.println(String("targetPower == tempOutputPower") + String(targetPower == tempOutputPower));
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
    // 返回逆变器功率
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
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",sendExecuteCommand【警告】异步读取空数据过多，需要重新进行查询模式,读取空数据次数为:") + String(readEmptyTimes));
        enterReadMode = true;
    }
    if (enterReadMode) {
        udp.beginPacket(_ip.c_str(), 48899);
        String readCommand = "WIFIKIT-214028-READ";
        // MessageOutput.println("send command:" + readCommand);
        udp.write(CommonUtils::stringToUint8_tArray(readCommand), readCommand.length());
        udp.endPacket();
        String okCommand = "+ok";
        udp.beginPacket(_ip.c_str(), 48899);
        // MessageOutput.println("send command:" + okCommand);
        udp.write(CommonUtils::stringToUint8_tArray(okCommand), okCommand.length());
        udp.endPacket();
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",sendExecuteCommand进入查询模式指令已下发,指令内容为: WIFIKIT-214028-READ\n+ok"));
    }
    // MessageOutput.println("command Length:" + String(command.length()));
    if (command.length() > 0) {
        udp.beginPacket(_ip.c_str(), 48899);
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",sendExecuteCommand下发功率设置指令,指令内容为:") + command);
        udp.write(CommonUtils::stringToUint8_tArray(command), command.length());
        udp.endPacket();
    }
}

// 发送命令
void DeyeInverter::sendCommand(String command, bool enterReadMode)
{
    // 发送命令的代码
    if (readErrorTimes > 10) {
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",sendCommand【警告】异步读取空数据过多，需要重新进行查询模式,读取空数据次数为:") + String(readErrorTimes));
        enterReadMode = true;
        readErrorTimes = 0;
    }
    if (enterReadMode) {
        udp.beginPacket(_ip.c_str(), 48899);
        String readCommand = "WIFIKIT-214028-READ";
        // MessageOutput.println("send command:" + readCommand);
        udp.write(CommonUtils::stringToUint8_tArray(readCommand), readCommand.length());
        udp.endPacket();
        String okCommand = "+ok";
        udp.beginPacket(_ip.c_str(), 48899);
        // MessageOutput.println("send command:" + okCommand);
        udp.write(CommonUtils::stringToUint8_tArray(okCommand), okCommand.length());
        udp.endPacket();
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",sendCommand进入查询模式指令已下发,指令内容为: WIFIKIT-214028-READ\n+ok"));
    }
    // MessageOutput.println("command Length:" + String(command.length()));
    if (command.length() > 0) {
        udp.beginPacket(_ip.c_str(), 48899);
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",sendCommand下发普通指令:") + command);
        udp.write(CommonUtils::stringToUint8_tArray(command), command.length());
        udp.endPacket();
    }
}

// 发送命令
void DeyeInverter::sendActiveCommand(String command, bool enterReadMode)
{
    if (readActiveEmptyTimes > 10) {
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",【警告】异步读取空数据过多，需要重新进行查询模式,读取空数据次数为:") + String(readActiveEmptyTimes));
        enterReadMode = true;
    }
    if (enterReadMode) {
        delay(100);
        activePowerUdp.beginPacket(_ip.c_str(), 48899);
        String readCommand = "WIFIKIT-214028-READ";
        // MessageOutput.println("send command:" + readCommand);
        activePowerUdp.write(CommonUtils::stringToUint8_tArray(readCommand), readCommand.length());
        activePowerUdp.endPacket();
        delay(100);
        String okCommand = "+ok";
        activePowerUdp.beginPacket(_ip.c_str(), 48899);
        // MessageOutput.println("send command:" + okCommand);
        activePowerUdp.write(CommonUtils::stringToUint8_tArray(okCommand), okCommand.length());
        activePowerUdp.endPacket();
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",进入查询模式指令已下发,指令内容为: WIFIKIT-214028-READ\n+ok"));
    }
    // MessageOutput.println("command Length:" + String(command.length()));
    if (command.length() > 0) {
        activePowerUdp.beginPacket(_ip.c_str(), 48899);
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",下发(当前功率查询)普通指令:") + command);
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
        // MessageOutput.println(String("read data length:") + String(readLength));
        if (readLength > 0) {
            char buffer[1024];
            activePowerUdp.read(buffer, readLength);
            String ret = String((char*)buffer).substring(0, readLength);
            MessageOutput.println(String("deviceSn:") + _deviceSn + String(",德业逆变器异步指令响应(当前功率查询):") + String(ret));
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
    while (millis() - start < 1000) { // 最多等待 300ms
        i++;
        int readLength = udp.parsePacket();
        // MessageOutput.println("read data length:" + String(readLength));
        if (readLength > 0) {
            char* buffer = (char*)malloc(256);
            udp.read(buffer, readLength);
            String ret = String((char*)buffer).substring(0, readLength);
            MessageOutput.println(String("deviceSn:") + _deviceSn + String(",德业逆变器异步指令响应:") + String(ret)+String("共读次数")+i);
            readEmptyTimes = 0;
            free(buffer);
            return ret;
        } else {
            readEmptyTimes++;
        }
        delay(5); // 防止 CPU 空转
    }
    readErrorTimes++;
    MessageOutput.println(String("deviceSn:") + _deviceSn + String(",德业逆变器异步指令响应读空一次，目前读空次数:")+String(readErrorTimes));
    // for (int i = 0; i < cycleTimes; i++) {
    //     delay(delayMillisecond);
    //     int readLength = udp.parsePacket();
    //     // MessageOutput.println("read data length:" + String(readLength));
    //     if (readLength > 0) {
    //         char buffer[1024];
    //         udp.read(buffer, readLength);
    //         String ret = String((char*)buffer).substring(0, readLength);
    //         MessageOutput.println(String("deviceSn:") + _deviceSn + String(",德业逆变器异步指令响应:") + String(ret));
    //         readEmptyTimes = 0;
    //         return ret;
    //     } else {
    //         readEmptyTimes++;
    //     }
    // }
    return "";
}

void DeyeInverter::handleResult(String result, int type)
{

    if (result.indexOf(_ip) >= 0 && type == DEVICESN) {
        _deviceSn = result.substring(result.lastIndexOf(",") + 1);
    } else if (result.indexOf("+ok") >= 0 && result.indexOf("0103") == 4) {
        result = result.substring(4);
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",resultType:") + String(type));
        String number = result.substring(6, 10);
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",number:") + number);
        int hexInt = CommonUtils::convertHexStrToInt(number);
        MessageOutput.println(String("deviceSn:") + _deviceSn + String(",hexInt:") + String(hexInt)); // MessageOutput.println("hexInt/10000.00:" + String(hexInt / 10000.00));
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
