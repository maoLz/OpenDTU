#pragma once

#include "IXmInverter.h"
#include "Configuration.h"
#include <AsyncJson.h>
#include "MessageOutput.h"




#define OUTPUTPOWER 53
#define MAXPOWER 40
#define RATEDPOWER 16
#define ACTIVEPOWER 86
#define DEVICESN 0

class DeyeInverter : public IXmInverter{

public:
    DeyeInverter();
    DeyeInverter(XM_INVERTER_T& inverter);;
    void init(AsyncWebServerRequest* request, CONFIG_T& config,JsonVariant& doc) override;
    void remove(AsyncWebServerRequest* request, CONFIG_T& config,JsonVariant& doc) override;
    void loadByConfig() override;
    bool isOpen() override;
    String getDeviceSn() override;
    void setInvertPower(int power,bool isWebRequest) override;
    void setInvertPower(int power) override;
    int getInvertPower() override;
    int getMaxPower() override;
    int getRatedPower() override;
    int getCommandPower() override;
    float getOutputPower() override;
    int getType() override;
    int getIndex() override;
    String getRemark() override;
    void openSmartStrategy(bool smartStrategy) override;

    void sendCommand(String command,bool enterReadMode);
    void sendExecuteCommand(String command,bool enterReadMode);
    void read53Power(int times,int targetPower);
    void sendActiveCommand(String command,bool enterReadMode);
    String syncRead(int delayMillisecond,int cycleTimes);
    String syncReadActive(int delayMillisecond,int cycleTimes);;
    void handleResult(String result,int type);

private:
    /**
     * 需要存放在配置的信息
     */
    String _deviceSn = "";
    String _ip = "";
    int _maxPower = 0;
    bool _isOpen = false;
    int _type = 1;
    int _index = 0;
    /**
     * 只放入内存的信息
     */
    int readEmptyTimes = -1;
    int readActiveEmptyTimes = -1;
    bool enterReadMode = true;
    int _outputPower = -1;
    int _activePower = -1;
    int _ratedPower= -1;

    WiFiUDP udp;

    WiFiUDP setCommandUdp;

    WiFiUDP activePowerUdp;





};
