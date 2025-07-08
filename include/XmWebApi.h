// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class XmWebApi {
public:
    void init(AsyncWebServer& server);
private:
    void onConfigGet(AsyncWebServerRequest* request);
    void onVersionGet(AsyncWebServerRequest* request);
    void onApsPost(AsyncWebServerRequest* request);
    void onShellyPost(AsyncWebServerRequest* request);
    void onSmartStrategyPost(AsyncWebServerRequest* request);
    void onSmartStrategyGet(AsyncWebServerRequest* reqeust);
    void onTest(AsyncWebServerRequest* reqeust);
    void onTest2(AsyncWebServerRequest* reqeust);
    bool isTrue(AsyncWebServerRequest* request,String key);
    void closeSmartStrategy(CONFIG_T& config);
    void onAddInvert(AsyncWebServerRequest* request);
};
