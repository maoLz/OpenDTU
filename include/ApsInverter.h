#pragma once

#include "IXmInverter.h"
#include "Configuration.h"
#include <AsyncJson.h>
#include "MessageOutput.h"


#define APS_GET_DEVICE_INFO "getDeviceInfo"

class ApsInverter : public IXmInverter{

public:
    ApsInverter();
    ApsInverter(XM_INVERTER_T& inverter);;
    void init(AsyncWebServerRequest* request, CONFIG_T& config,JsonVariant& doc) override;
    void remove(AsyncWebServerRequest* request, CONFIG_T& config,JsonVariant& doc) override;
    void loadByConfig() override;
    bool isOpen() override;
    String getDeviceSn() override;
    void setInvertPower(int power) override;
    void setInvertPower(int power,bool isWebRequest) override;
    int getInvertPower() override;
    int getMaxPower() override;
    int getCommandPower() override;
    float getOutputPower() override;
    int getType() override;
    int getIndex() override;
    int getRatedPower() override;
    String getRemark() override;
    void openSmartStrategy(bool smartStrategy);


private:
    /**
     * 需要存放在配置的信息
     */
    String _deviceSn ="";
    String _ip = "";
    int _maxPower = 0;
    bool _isOpen = false;
    int _type = 1;
    int _index = 0;
    /**
     * 只放入内存的信息
     */
    String _hostName = "";



    /**
     * APS特殊方法
     */
    String requestAPI(String api,JsonDocument& doc);
};
