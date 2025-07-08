#pragma once

#include "Configuration.h"
#include <AsyncJson.h>
#include "XmException.h"


class XmAps {

public:
    void init(CONFIG_T& config,String ip);
    void reload();
    String deviceSn;
    String hostName;
    String ip;
    bool open;
    ExceptionType lastException;
    bool isOnline;
    int getInvertPower();
    float getOutputData();
    void setInvertPower(int power);
    String getDeviceSn();
private:
    String requestAPI(String api,JsonDocument& doc);
};

extern XmAps Aps;
