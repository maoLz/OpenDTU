#include "Configuration.h"
#include "WebApi.h"
#include <AsyncJson.h>
#include "MessageOutput.h"
#include "XmWebApi.h"
#include "XmMqttClass.h"
#include "XmShelly.h"
#include "XmAps.h"
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include "XmException.h"
#include "ApsInverter.h"
#include "XmSmartStrategy.h"






void XmWebApi::init(AsyncWebServer& server)
{
    using std::placeholders::_1;

    //test API
    server.on("/rest/api/config",HTTP_GET,std::bind(&XmWebApi::onConfigGet,this,_1));
    server.on("/rest/api/version",HTTP_GET,std::bind(&XmWebApi::onVersionGet,this,_1));
    server.on("/rest/api/test",HTTP_POST,std::bind(&XmWebApi::onTest,this,_1));

    //dev API
    server.on("/rest/api/addInvert",HTTP_POST,std::bind(&XmWebApi::onAddInvert,this,_1));

    //prod API
    server.on("/rest/api/aps",HTTP_POST,std::bind(&XmWebApi::onApsPost,this,_1));
    server.on("/rest/api/shelly",HTTP_POST,std::bind(&XmWebApi::onShellyPost,this,_1));
    server.on("/rest/api/smartStrategy",HTTP_POST,std::bind(&XmWebApi::onSmartStrategyPost,this,_1));
    server.on("/rest/api/localData",HTTP_POST,std::bind(&XmWebApi::onSmartStrategyGet,this,_1));


}

void XmWebApi::onAddInvert(AsyncWebServerRequest* request){
    ApsInverter aps;
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto guard = Configuration.getWriteGuard();
    CONFIG_T& config = guard.getConfig();
    auto& root = response->getRoot();
    try{
        aps.init(request,config,root);

    }catch(CustomException& e){
        Configuration.read();
        root["msg"] = e.msg();
        root["code"] = e.wrong();
    }
    WebApi.sendJsonResponse(request,response,__FUNCTION__,__LINE__);
}

/**
    测试接口
    onConfigGet
    @param request
    @return 配置信息获取

 */
void XmWebApi::onConfigGet(AsyncWebServerRequest* request){
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& config = Configuration.get();
    auto& root = response->getRoot();
    root["SmartStrategy"] = config.Xm.SmartStrategy;
    root["XmInterval"] = config.Xm.XmInterval;
    root["maxPower"] = config.Xm.maxPower;

    JsonObject aps = root["Aps"].to<JsonObject>();
    aps["Ip"] = IPAddress(config.Xm.Aps.Ip).toString();
    aps["DeviceSn"] = config.Xm.Aps.DeviceSn;
    aps["HostName"] = config.Xm.Aps.HostName;
    aps["open"] = config.Xm.Aps.open;

    JsonObject shelly = root["Shelly"].to<JsonObject>();
    shelly["DeviceSn"] = config.Xm.Shelly.DeviceSn;
    shelly["HostName"] = config.Xm.Shelly.HostName;
    shelly["IsPro"] = config.Xm.Shelly.IsPro;
    shelly["open"] = config.Xm.Shelly.open;

    JsonArray inverters = root["Inverters"].to<JsonArray>();
    for (uint8_t i = 0; i < XM_INVERTER_MAX; i++) {
        JsonObject inverter = inverters.add<JsonObject>();
        inverter["DeviceSn"] = config.Xm.Inverters[i].DeviceSn;
        inverter["Ip"] = IPAddress(config.Xm.Inverters[i].Ip).toString();
        inverter["maxPower"] = config.Xm.Inverters[i].MaxPower;
        inverter["open"] = config.Xm.Inverters[i].Open;
    }
    JsonObject memory = root["Memory"].to<JsonObject>();

    // Write attributes from XmShelly
    JsonObject shellyMemory = memory["Shelly"].to<JsonObject>();
    shellyMemory["DeviceSn"] = config.Xm.Shelly.DeviceSn;
    shellyMemory["HostName"] = config.Xm.Shelly.HostName;
    shellyMemory["IsPro"] = config.Xm.Shelly.IsPro;
    shellyMemory["open"] = config.Xm.Shelly.open;

    // Write attributes from XmAps
    JsonObject apsMemory = memory["Aps"].to<JsonObject>();
    apsMemory["Ip"] = IPAddress(config.Xm.Aps.Ip).toString();
    apsMemory["DeviceSn"] = config.Xm.Aps.DeviceSn;
    apsMemory["HostName"] = config.Xm.Aps.HostName;
    apsMemory["open"] = config.Xm.Aps.open;
    WebApi.sendJsonResponse(request,response,__FUNCTION__,__LINE__);
}

/**
    测试接口
    onVersionGet
    @param request
    @return 版本信息获取
 */
void XmWebApi::onVersionGet(AsyncWebServerRequest* request){
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    try{
        root["invertPower"] = Aps.getInvertPower();
    }catch(CustomException& e){
        root["msg"] =e.msg();
        root["code"] = e.wrong();
    }
    //root["maxOutpower"]  = Aps.getInvertPower();


    WebApi.sendJsonResponse(request,response,__FUNCTION__,__LINE__);
}

/**
    正式接口 (开启逆变器本地模式接口)
    onApsPost
    @param request
    @param ip aps IP地址
    @param open 是否开启本地模式
    @return
 */
void XmWebApi::onApsPost(AsyncWebServerRequest* request){
    if(!WebApi.checkCredentials(request)){
        return;
    }
    auto guard = Configuration.getWriteGuard();
    CONFIG_T& config = guard.getConfig();
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    try{
        if(!request->hasParam("open",true)){
             throw CustomException("open param not found.",API_PARAMS_NOT_FOUND);
        }
        bool open = isTrue(request,"open");

        Aps.open = open;
        if(open){
            if(!request ->hasParam("ip",true)){
                throw CustomException("ip param not found.",API_PARAMS_NOT_FOUND);
            }
            String ip = request->getParam("ip",true)->value();
            IPAddress ipaddress;
            if(!ipaddress.fromString(ip)){
                throw CustomException("ip format wrong.",API_APS_IP_FORMAT_ERROR);
            }
            config.Xm.Aps.open = open;
            Aps.init(config,ip);
            config.Xm.Aps.Ip[0] = ipaddress[0];
            config.Xm.Aps.Ip[1] = ipaddress[1];
            config.Xm.Aps.Ip[2] = ipaddress[2];
            config.Xm.Aps.Ip[3] = ipaddress[3];
            root["deviceSn"] = Aps.deviceSn;
        }else{
            config.Xm.Aps.open = open;
            MessageOutput.println("config open is false");
            closeSmartStrategy(config);
            try{
                Aps.setInvertPower(config.Xm.maxPower);
            }catch(CustomException& e){

            }
        }
        if(!Configuration.write()){
            throw CustomException("dtu config write wrong.",DTU_CONF_WRITE_ERROR);
        }
        root["code"] = 0;
    }catch(CustomException& e){
        Configuration.read();
        Aps.reload();
        root["msg"] = e.msg();
        root["code"] = e.wrong();
    }
    WebApi.sendJsonResponse(request,response,__FUNCTION__,__LINE__);
}

/**
    正式接口 (开启shelly本地模式接口)
    onShellyPost
    @param request
    @param shellySn shelly设备SN
    @param isPro shelly设备类型
    @param open 是否开启本地模式
    @return
 */
void XmWebApi::onShellyPost(AsyncWebServerRequest* request){
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    auto guard = Configuration.getWriteGuard();
    CONFIG_T& config = guard.getConfig();
    try{
        if(!request ->hasParam("shellySn",true) || !request->hasParam("isPro",true) || !request->hasParam("open",true)){
            throw CustomException("shellySn/isPro param not found.",API_PARAMS_NOT_FOUND);
        }
        String shellySn = request->getParam("shellySn",true)->value();
        shellySn.trim();
        if(shellySn.length()== 0){
            throw CustomException("shellySn/isPro param not found.",API_PARAMS_NOT_FOUND);
        }
        shellySn.toLowerCase();
        bool isPro = isTrue(request,"isPro");
        bool open = isTrue(request,"open");
        Shelly.open = open;
        config.Xm.Shelly.IsPro =  isPro;
        config.Xm.Shelly.open = open;
        if(open){
            Shelly.init(config,shellySn,isPro);
            root["hostName"] = Shelly.hostName;
            root["totalPower"] = Shelly.getTotalPower();
        }else{
            bool oldSmartStrategy = SmartStrategy.getOpen();
            closeSmartStrategy(config);
            SmartStrategy.setOpen(false);
            try{
                if(oldSmartStrategy){
                    SmartStrategy.setInverterMaxPower(oldSmartStrategy);
                }
            }catch(CustomException& e){

            }
        }
        strlcpy(config.Xm.Shelly.DeviceSn, shellySn.c_str(),sizeof(config.Xm.Shelly.DeviceSn));
        if(!Configuration.write()){
            throw CustomException("dtu config write wrong.",DTU_CONF_WRITE_ERROR);
        }
        root["deviceSn"] = Shelly.deviceSn;
        root["code"] = 0;
    }catch(CustomException& e){
        Configuration.read();
        Shelly.reload();
        root["msg"] = e.msg();
        root["code"] = e.wrong();
    }
    WebApi.sendJsonResponse(request,response,__FUNCTION__,__LINE__);
}

// void XmWebApi::onDeyePost(AsyncWebServerRequest* request){
//     AsyncJsonResponse* response = new AsyncJsonResponse();
//     auto& root = response->getRoot();
//     auto guard = Configuration.getWriteGuard();
//     CONFIG_T& config = guard.getConfig();
//     try{

//         root["code"] = 0;
//         WebApi.sendJsonResponse(request,response,__FUNCTION__,__LINE__);
//     }catch(CustomException& e){
//         root["msg"] = e.msg();
//         root["code"] = e.wrong();
//     }
// }

/**
    正式接口 (开启智能策略接口)
    onSmartStrategyPost
    @param request
    @param smartStrategy 是否开启智能策略
    @return
 */
void XmWebApi::onSmartStrategyPost(AsyncWebServerRequest* request){
    if(!WebApi.checkCredentials(request)){
        return;
    }
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    auto guard = Configuration.getWriteGuard();
    CONFIG_T& config = guard.getConfig();
    try{
        if(!request ->hasParam("smartStrategy",true)){
            throw CustomException("smartStrategy param not found.",API_PARAMS_NOT_FOUND);
        }
        config.Xm.SmartStrategy = isTrue(request,"smartStrategy");
        config.Xm.maxPower =800;
        XmMqtt.reload(config);
        if(!Configuration.write()){
            throw CustomException("dtu config write wrong.",DTU_CONF_WRITE_ERROR);
        }
        root["smartStrategy"] = config.Xm.SmartStrategy;
        root["maxPower"] = config.Xm.maxPower;
        root["code"] = 0;
        if(config.Xm.SmartStrategy){
            Aps.setInvertPower(30);
        }else{
            Aps.setInvertPower(800);
        }
    }catch(CustomException& e){
        root["msg"] = e.msg();
        root["code"] = e.wrong();
    }
    WebApi.sendJsonResponse(request,response,__FUNCTION__,__LINE__);
}

/**
    正式接口 获取智能策略数据
    onSmartStrategyGet
    @param request
    @return
 */
void XmWebApi::onSmartStrategyGet(AsyncWebServerRequest* request){
    if(!WebApi.checkCredentialsReadonly(request)){
        return;
    }
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    auto& config =Configuration.get();
    bool apsOpen = false;
    bool shellyOpen = false;
    try{
        JsonObject apsObject = root["aps"].to<JsonObject>();
        JsonObject shellyObject = root["shelly"].to<JsonObject>();
        apsObject["deviceSn"]="";
        shellyObject["deviceSn"]="";

        if(request->hasParam("apsSn",true)){
            String apsSn = request->getParam("apsSn",true)->value();
            if(apsSn.equals(String(config.Xm.Aps.DeviceSn))){
                //测试通信是否正常
                apsObject["deviceSn"] = apsSn;
                apsOpen = config.Xm.Aps.open;
                if(apsOpen){
                    int currentPower = Aps.getOutputData();
                    apsObject["currentPower"] = currentPower;
                }
            }
            time_t now;
            time(&now);
            apsObject["timestamp"] = now;
        }
        if(request->hasParam("shellySn",true)){
            String shellySn = request->getParam("shellySn",true)->value();
            shellySn.toLowerCase();
            if(shellySn.equals(String(config.Xm.Shelly.DeviceSn))){
                //测试通信是否正常
                shellyObject["deviceSn"] = shellySn;
                shellyOpen = config.Xm.Shelly.open;
                if(shellyOpen){
                    int totalActPower = Shelly.getTotalPower();
                    shellyObject["totalPower"] = totalActPower;
                }
            }
            time_t now;
            time(&now);
            shellyObject["timestamp"] = now;
        }
        //未传入apsOpen,则直接返回本地模式状态关闭
        apsObject["open"] = apsOpen;
        shellyObject["open"] = shellyOpen;
        root["code"] = 0;
    }catch(CustomException& e){
        root["msg"] = e.msg();
        root["code"] = e.wrong();
    }
    root["smart"] = config.Xm.SmartStrategy;
    WebApi.sendJsonResponse(request,response,__FUNCTION__,__LINE__);
}

void XmWebApi::onTest(AsyncWebServerRequest* request){
    if(!WebApi.checkCredentialsReadonly(request)){
        return;
    }
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    try{
        root["code"] = "1123123";
    }catch(CustomException& e){
        root["msg"] = e.msg();
        root["code"] = e.wrong();
    }
    WebApi.sendJsonResponse(request,response,__FUNCTION__,__LINE__);
}


void XmWebApi::onTest2(AsyncWebServerRequest* request){
    if(!WebApi.checkCredentialsReadonly(request)){
        return;
    }
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    auto& config =Configuration.get();
    try{
        root["code"] = "1123123";
    }catch(CustomException& e){
        root["msg"] = e.msg();
        root["code"] = e.wrong();
    }
    WebApi.sendJsonResponse(request,response,__FUNCTION__,__LINE__);
}


bool XmWebApi::isTrue(AsyncWebServerRequest* request,String key){
    String str = request->getParam(key,true)->value();
    str.toLowerCase();
    return String("true").equals(str);
}


void XmWebApi::closeSmartStrategy(CONFIG_T& config){
    config.Xm.SmartStrategy = false;
    XmMqtt.reload(config);
}
