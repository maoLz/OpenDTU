#pragma once

#include <ESPmDNS.h>
#include <WiFi.h>
#include "Configuration.h"



class XmShelly{

public:
    void init(CONFIG_T& config,String deviceSn,bool isPro);
    void openMDNS(CONFIG_T& config);
    void reload();
    String deviceSn;
    String hostName;
    bool open;
    bool isPro;
    bool isOnline;
    int lastException;
    int getTotalPower();
    void initShellyHostName(CONFIG_T& config);
};

extern XmShelly Shelly;
