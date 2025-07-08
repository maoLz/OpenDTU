#pragma once

#include "Configuration.h"
#include <TaskSchedulerDeclarations.h>
#include "XmException.h"
#include <espMqttClient.h>
#include <frozen/map.h>
#include <frozen/string.h>

class XmMqttClass{

public:
    XmMqttClass();
    void init(Scheduler& scheduler);
    String subTopic;
    String pubTopic;
    String pubStatusTopic;
    String apsTopic;
    String dtuSn;
    bool smartStrategy;
    uint32_t xmInterVal;
    int maxPower;
    void startTask();
    void reload(CONFIG_T config);
    int smartStrategyFunc(int totalPower,int invertPower);
    void buildCommandResponse(String uuid,bool isSuccess,const ExceptionType exception,String message);
    void buildSuccess(String uuid);
    void buildLoopMsg(bool isSuccess,const ExceptionType exception,String message);

private:
    Task _loopTask;
    void loop();
    void subscribeTopics();
    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, const size_t len, const size_t index, const size_t total);
};

extern XmMqttClass XmMqtt;
