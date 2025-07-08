#include "XmAps.h"
#include <AsyncJson.h>
#include <HTTPClient.h>
#include "XmException.h"
#include "MessageOutput.h"

void XmAps::init(CONFIG_T& config,String ip){
    hostName = String("http://") + ip + String(":8050");
    deviceSn = getDeviceSn();
    strlcpy(config.Xm.Aps.HostName, hostName.c_str(),sizeof(config.Xm.Aps.HostName));
    strlcpy(config.Xm.Aps.DeviceSn, deviceSn.c_str(),sizeof(config.Xm.Aps.DeviceSn));
}

void XmAps::reload(){
    auto& config = Configuration.get();
    deviceSn = String(config.Xm.Aps.DeviceSn);
    hostName = String(config.Xm.Aps.HostName);
    open = config.Xm.Aps.open;
}

float XmAps::getOutputData(){
    JsonDocument doc;
    requestAPI("getOutputData",doc);
    return doc["data"]["p1"].as<float>() + doc["data"]["p2"].as<float>();

}

String XmAps::requestAPI(String api,JsonDocument& doc){
    String apiUrl = hostName + String("/") +api;
    HTTPClient _httpClient;
    _httpClient.begin(apiUrl.c_str());
    _httpClient.setConnectTimeout(2000);
    int httpResponseCode = _httpClient.GET();
    if(httpResponseCode > 0){
        isOnline =true;
        String payload = _httpClient.getString();
        char buf[payload.length() + 1];
        strcpy(buf,payload.c_str());
        deserializeJson(doc, buf);
        return payload;
    }else{
        isOnline = false;
        String errorMsg = String("request url:") + apiUrl + String("response Code") + httpResponseCode;
        throw CustomException(errorMsg ,APS_REQUEST_ERROR);
    }
}

String XmAps::getDeviceSn(){
    JsonDocument doc;
    String payload = requestAPI("getDeviceInfo",doc);
    if(!doc.containsKey("deviceId") || doc["deviceId"].as<String>().length() == 0){
        throw CustomException(payload,APS_DEVICE_SN_GET_ERROR);
    }
    return doc["deviceId"];
}

int XmAps::getInvertPower(){
    JsonDocument doc;
    String payload = requestAPI("getMaxPower",doc);
    return doc["data"]["power"];

}

void XmAps::setInvertPower(int power){
    String api = String("setMaxPower?p=") + String(power);
    JsonDocument doc;
    String payload = requestAPI(api,doc);
    MessageOutput.println(String("XmAps,setInverterPower")+api);
    if(!String("SUCCESS").equals(doc["message"].as<String>())){
        throw CustomException(payload,APS_POWER_SET_ERROR);
    }
}
XmAps Aps;



