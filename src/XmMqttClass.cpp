#include "MqttSettings.h"
#include "XmMqttClass.h"
#include "MessageOutput.h"
#include <ctime>
#include <ArduinoJson.h>
#include "Configuration.h"
#include "__compiled_constants.h"
#include "XmShelly.h"
#include "XmAps.h"

XmMqttClass::XmMqttClass()
    : _loopTask(TASK_IMMEDIATE,TASK_FOREVER,std::bind(&XmMqttClass::loop,this)){

}

void XmMqttClass::init(Scheduler& scheduler){
        String const& prefix = MqttSettings.getPrefix();
        dtuSn = Configuration.getDtuSn();
        CONFIG_T config = Configuration.get();
        subTopic = prefix+"dtu/" + dtuSn + "/config";
        pubTopic = "dtu/" + dtuSn + "/beat";
        pubStatusTopic = "dtu/" + dtuSn + "/status";
        apsTopic = "dtu/" + dtuSn + "/aps";
        subscribeTopics();
        Shelly.reload();
        Aps.reload();
        scheduler.addTask(_loopTask);
        reload(config);
        _loopTask.setInterval(xmInterVal * TASK_SECOND);
        _loopTask.enable();
}

void XmMqttClass::reload(CONFIG_T config){
     xmInterVal = config.Xm.XmInterval;
     smartStrategy = config.Xm.SmartStrategy;
     maxPower = config.Xm.maxPower;
}

void XmMqttClass::startTask(){
    if(smartStrategy){
        if(Aps.deviceSn.length() == 0){
            throw CustomException("aps devciesn is empty.",DTU_START_SMART_ERROR);
        }
        if(Shelly.hostName.length() == 0){
            throw CustomException("shelly hostName is empty.",DTU_START_SMART_ERROR);
        }
        _loopTask.enable();
    }else{
        _loopTask.disable();
    }
}


void XmMqttClass::loop(){
    // _loopTask.setInterval(xmInterVal * TASK_SECOND);

    // if(!smartStrategy || !Shelly.open ||!Aps.open){
    //     _loopTask.forceNextIteration();
    //     return;
    // }
    // int totalPower = 0;
    // try{
    //         MessageOutput.println("mqtt getTotalPower");
    //     totalPower = Shelly.getTotalPower();
    //     MessageOutput.print("shelly total power :");
    //     MessageOutput.println(totalPower);
    // }catch (CustomException& e){
    //     MessageOutput.println("shelly get power error.");
    //     return;
    // }
    // try{
    //     int outPutPower = Aps.getOutputData();
    //     int newInvertPower  =smartStrategyFunc(totalPower,outPutPower);
    //     MessageOutput.print("invert current power:");
    //     MessageOutput.print(outPutPower);
    //     MessageOutput.print(",Invert need set power:");
    //     MessageOutput.println(newInvertPower);
    //     if(newInvertPower != outPutPower){
    //         Aps.setInvertPower(newInvertPower);
    //     }
    // }catch(CustomException& e){
    //     MessageOutput.println(e.msg());
    //     return;
    // }
}



int XmMqttClass::smartStrategyFunc(int totalPower,int invertPower){
    //当前空间功率 + 微逆最大设置功率
    int modifyPower = totalPower + invertPower;
    if(modifyPower < 30){
        modifyPower = 30;
    }else if(modifyPower > maxPower){
        modifyPower = maxPower;
    }
    return modifyPower;
}

void XmMqttClass::subscribeTopics(){

    MessageOutput.println("subscribe succ");
}

void XmMqttClass::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, const size_t len, const size_t index, const size_t total){
    // std::string strValue(reinterpret_cast<const char*>(payload), len);
    // JsonDocument doc;
    // MessageOutput.println(strValue.c_str());

    // const DeserializationError error = deserializeJson(doc, strValue.c_str());
    // if (error) {
    //     buildCommandResponse("",false,MQTT_MSG_FORMAT_ERROR,"deserializate message to json error.");
    //     return;
    // }
    // auto guard = Configuration.getWriteGuard();
    // CONFIG_T& config = guard.getConfig();
    // String commandId = String(doc["commandId"]);
    // if(doc.containsKey("apsIp")){
    //     String apsIp = String(doc["apsIp"]);
    //     IPAddress ipaddress;
    //     if(!ipaddress.fromString(apsIp)){
    //         buildCommandResponse(commandId,false,APS_IP_ERROR,apsIp + "APS IP address is error.cannot parse.");
    //         return;
    //     }
    //     config.Xm.Aps.Ip[0] = ipaddress[0];
    //     config.Xm.Aps.Ip[1] = ipaddress[1];
    //     config.Xm.Aps.Ip[2] = ipaddress[2];
    //     config.Xm.Aps.Ip[3] = ipaddress[3];
    //     try{
    //         Aps.init(config);
    //     }catch(CustomException &e){
    //         Configuration.write();
    //         buildCommandResponse(commandId,false,e.wrong(),e.msg());
    //         return;
    //     }
    // }else if(doc.containsKey("shellySn")){
    //     String shellySn = String(doc["shellySn"]);
    //     bool isPro = doc["isPro"];
    //     if(!doc.containsKey("isPro")){
    //         buildCommandResponse(commandId,false,SHELLY_IS_PRO_NOT_FOUND,"Shelly isPro params not found.");
    //         return;
    //     }
    //     strlcpy(config.Xm.Shelly.DeviceSn, shellySn.c_str(),sizeof(config.Xm.Shelly.DeviceSn));
    //     config.Xm.Shelly.IsPro = isPro;
    //     try{
    //         Shelly.init(config);
    //     }catch(CustomException &e){
    //         Configuration.write();
    //         buildCommandResponse(commandId,false,e.wrong(),e.msg());
    //         return;
    //     }
    // }else if(doc.containsKey("smartStrategy")){
    //     MessageOutput.println("smartStrategy if else...");
    //     bool smartStrategy = doc["smartStrategy"];
    //     uint32_t xmInterval = doc["xmInterval"];
    //     config.Xm.SmartStrategy = smartStrategy;
    //     config.Xm.XmInterval = xmInterval;
    //     reload(config);
    //     //startTask();
    // }else{
    //     buildCommandResponse(commandId,false,MQTT_UNKNOWN_MSG,"mqtt message unknown, no handle.");
    //     return;
    // }
    // if(!Configuration.write()){
    //     buildCommandResponse(commandId,false,DTU_CONF_WRITE_ERROR,"config write error.");
    //     return;
    // }
    // buildSuccess(commandId);
    return;
}

void XmMqttClass::buildCommandResponse(String uuid,bool isSuccess,const ExceptionType exception,String message){
    JsonDocument doc;
    doc["dtuSn"] = dtuSn;
    doc["commandId"] = uuid;
    time_t now;
    time(&now);
    doc["timestamp"] = now;
    JsonObject rst = doc["data"].to<JsonObject>();
    doc["success"] = isSuccess;
    rst["error_code"] = exception | 0;
    rst["msg"] = message;
    String buffer;
    serializeJson(doc,buffer);
    MqttSettings.publish(pubTopic,buffer);
}

void XmMqttClass::buildSuccess(String uuid){
    JsonDocument doc;
    doc["dtuSn"] = dtuSn;
    doc["commandId"] = uuid;
    time_t now;
    time(&now);
    doc["timestamp"] = now;
    doc["success"] = true;
    String buffer;
    serializeJson(doc,buffer);
    MqttSettings.publish(pubTopic,buffer);
}

void XmMqttClass::buildLoopMsg(bool isSuccess,const ExceptionType exception,String message){
    JsonDocument doc;
    doc["dtuSn"] = dtuSn;
    time_t now;
    time(&now);
    doc["timestamp"] = now;
    JsonObject rst = doc["data"].to<JsonObject>();
    doc["success"] = isSuccess;
    rst["code"] = exception;
    rst["message"] = message;
    String buffer;
    serializeJson(doc,buffer);
    MqttSettings.publish(pubTopic,buffer);
}


XmMqttClass XmMqtt;
