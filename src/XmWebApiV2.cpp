
#include "ApsInverter.h"
#include "CommonUtils.h"
#include "Configuration.h"
#include "DeyeInverter.h"
#include "WebApi.h"
#include "XmException.h"
#include "XmShelly.h"
#include "XmSmartStrategy.h"

void XmWebApiV2::init(AsyncWebServer& server)
{

    using std::placeholders::_1;

    server.on("/rest/v2/addInverter", HTTP_POST, std::bind(&XmWebApiV2::onAddInverter, this, _1));

    server.on("/rest/v2/removeInverter", HTTP_POST, std::bind(&XmWebApiV2::onRemoveInverter, this, _1));

    server.on("/rest/v2/spaceData", HTTP_POST, std::bind(&XmWebApiV2::onLoadConfig, this, _1));

    server.on("/rest/v2/spaceDataForDev", HTTP_POST, std::bind(&XmWebApiV2::spaceDataForDev, this, _1));


    server.on("/rest/v2/smartStrategy", HTTP_POST, std::bind(&XmWebApiV2::smartStrategy, this, _1));

    server.on("/rest/v2/setMaxPower", HTTP_POST, std::bind(&XmWebApiV2::setMaxPower, this, _1));
}

void XmWebApiV2::onAddInverter(AsyncWebServerRequest* request)
{
    esp_task_wdt_init(30, true);
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    try {
        if (!request->hasParam("type", true) || !request->hasParam("spaceInverterSn", true) || !request->hasParam("ip", true)) {
            throw CustomException("params is not valid.", API_PARAMS_NOT_FOUND);
        }
        int type = request->getParam("type", true)->value().toInt();
        if (type != INVERTER_TYPE_APS && type != INVERTER_TYPE_DEYE) {
            throw CustomException("type is not valid.", API_PARAMS_NOT_FOUND);
        }
        IXmInverter* inverter;
        if (type == INVERTER_TYPE_APS) {
            inverter = new ApsInverter();
        } else if (type == INVERTER_TYPE_DEYE) {
            inverter = new DeyeInverter();
        }
        auto guard = Configuration.getWriteGuard();
        CONFIG_T& config = guard.getConfig();
        inverter->init(request, config, root);
        String currentDeviceSn = inverter->getDeviceSn();
        esp_task_wdt_reset();
        bool oldOpen = SmartStrategy.getOpen();
        if(oldOpen){
            config.Xm.SmartStrategy = false;
            SmartStrategy.setOpen(false);
            auto inverters = SmartStrategy.inverters;
            auto inverterNumber = SmartStrategy.getInverterCount();
            for (int i = 0; i < inverterNumber; i++) {
                if (inverters[i] != nullptr && inverters[i]->getDeviceSn() != "") {
                    int needSetPower = inverters[i]->getMaxPower();
                    String deviceSn = inverters[i]->getDeviceSn();
                    if(needSetPower <= 0){
                        continue;
                    }
                    try {
                        if(oldOpen &&  currentDeviceSn.compareTo(deviceSn) != 0){
                            inverters[i]->setInvertPower(needSetPower, true);
                        }
                    } catch (CustomException& e) {
                    }
                }
            }
        }
    } catch (CustomException& e) {
        root["msg"] = e.msg();
        root["code"] = -1;
    } catch (...) {
        root["msg"] = String("server fail");
        root["code"] = -1;
    }
    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void XmWebApiV2::onRemoveInverter(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    try {
        if (!request->hasParam("deviceSn", true)) {
            throw CustomException("deviceSn param not found.", PARAMS_NOT_FOUND);
        }
        IXmInverter* inverter;
        String deviceSn = request->getParam("deviceSn", true)->value();
        inverter = SmartStrategy.getInverterByDeviceSn(deviceSn);
        if (inverter != nullptr) {
            bool oldSmartStrategy = SmartStrategy.getOpen();
            SmartStrategy.setOpen(false);
            auto guard = Configuration.getWriteGuard();
            Configuration.deleteXMInverterByDeviceSn(deviceSn.c_str());
            CONFIG_T& config = guard.getConfig();
            config.Xm.SmartStrategy = false;
            if (!Configuration.write()) {
                throw CustomException("dtu config write wrong.", DTU_CONF_WRITE_ERROR);
            }
            try{
                inverter->remove(request,config,root);
            }catch(...){

            }
             try{
                if(oldSmartStrategy){
                    //本地智能策略之前开启时，才恢复逆变器最大功率
                    SmartStrategy.setInverterMaxPower(oldSmartStrategy);
                }else{
                    if(inverter->getType() == INVERTER_TYPE_APS){
                        int needSetPower = inverter->getMaxPower();
                        inverter->setInvertPower(needSetPower, true);
                    }
                }
            }catch(...){
            }
            SmartStrategy.removeInverter(inverter->getDeviceSn());
            SmartStrategy.setOpen(false);
        }
        root["code"] = 0;
    } catch (CustomException& e) {
        Configuration.read();
        SmartStrategy.loadByConfig();
        root["msg"] = e.msg();
        root["code"] = -1;
    } catch (...) {
        Configuration.read();
        SmartStrategy.loadByConfig();
        root["msg"] = String("server fail");
        root["code"] = -1;
    }
    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void XmWebApiV2::spaceDataForDev(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    root["version"] = "1.0.1";
    root["smartStrategy"] = SmartStrategy.getOpen();
    root["interval"] = SmartStrategy.getInterval();
    root["maxPower"] = SmartStrategy.maxPower;
    try {

        auto data = root["inverters"].to<JsonArray>();
        for (int i = 0; i < XM_INVERTER_MAX; i++) {
            if (SmartStrategy.inverters[i] != nullptr) {
                String deviceSn = SmartStrategy.inverters[i]->getDeviceSn();
                auto inverter = data.add<JsonObject>();
                try {
                    esp_task_wdt_reset();
                    inverter["deviceSn"] = deviceSn;
                    inverter["type"] = SmartStrategy.inverters[i]->getType();
                    inverter["index"] = SmartStrategy.inverters[i]->getIndex();
                    inverter["maxPower"] = SmartStrategy.inverters[i]->getMaxPower();
                    inverter["outputPower"] = SmartStrategy.inverters[i]->getInvertPower();
                    esp_task_wdt_reset();
                    inverter["commandPower"] = SmartStrategy.inverters[i]->getCommandPower();
                    inverter["ratedPower"] = SmartStrategy.inverters[i]->getRatedPower();

                    // inverter["remark"] = SmartStrategy.inverters[i]->getRemark();
                    inverter["status"] = "online";
                } catch (CustomException& e) {
                    inverter["status"] = "offline";
                }
            }
        }
        if (Shelly.open) {
            JsonObject shelly = root["shelly"].to<JsonObject>();
            try {
                shelly["deviceSn"] = Shelly.deviceSn;
                shelly["open"] = Shelly.open;
                shelly["isPro"] = Shelly.isPro;
                shelly["totalPower"] = Shelly.getTotalPower();
                shelly["status"] = "online";
            } catch (CustomException& e) {
                shelly["status"] = "offline";
            }
        }
        time_t now;
        time(&now);
        root["timestamp"] = now;
        root["code"] = 0;
    } catch (CustomException& e) {
        root["msg"] = e.msg();
        root["code"] = -1;
    } catch (...) {
        root["msg"] = String("server fail");
        root["code"] = -1;
    }
    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}


void XmWebApiV2::onLoadConfig(AsyncWebServerRequest* request)
{
    esp_task_wdt_init(30, true);
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    root["smartStrategy"] = SmartStrategy.getOpen();
    try {
        auto data = root["inverters"].to<JsonArray>();
        for (int i = 0; i < XM_INVERTER_MAX; i++) {
            esp_task_wdt_reset();
            if (SmartStrategy.inverters[i] != nullptr) {
                String deviceSn = SmartStrategy.inverters[i]->getDeviceSn();
                auto inverter = data.add<JsonObject>();
                try {
                    inverter["deviceSn"] = deviceSn;
                    inverter["type"] = SmartStrategy.inverters[i]->getType();
                    inverter["outputPower"] = SmartStrategy.inverters[i]->getInvertPower();
                    // inverter["remark"] = SmartStrategy.inverters[i]->getRemark();
                    inverter["status"] = "online";
                } catch (CustomException& e) {
                    inverter["status"] = "offline";
                }
            }
        }
        if (Shelly.open) {
            JsonObject shelly = root["shelly"].to<JsonObject>();
            try {
                shelly["deviceSn"] = Shelly.deviceSn;
                shelly["isPro"] = Shelly.isPro;
                shelly["totalPower"] = Shelly.getTotalPower();
                shelly["open"] = Shelly.open;
                shelly["status"] = "online";
            } catch (CustomException& e) {
                shelly["open"] = false;
                shelly["status"] = "offline";
            }
        }
        time_t now;
        time(&now);
        root["timestamp"] = now;
        root["code"] = 0;
    } catch (CustomException& e) {
        root["msg"] = e.msg();
        root["code"] = -1;
    } catch (...) {
        root["msg"] = String("server fail");
        root["code"] = -1;
    }
    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void XmWebApiV2::setMaxPower(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    try {
        if (!request->hasParam("maxPower", true)) {
            throw CustomException("maxPower param not found.", API_PARAMS_NOT_FOUND);
        }
        struct tm timeinfo;
        getLocalTime(&timeinfo, 5);
        String timeLog = String("[") + String(timeinfo.tm_year + 1900) + String("-")
            + String(timeinfo.tm_mon + 1)
            + String("-")
            + String(timeinfo.tm_mday)
            + String(" ")
            + String(timeinfo.tm_hour)
            + String(":")
            + String(timeinfo.tm_min)
            + String(":")
            + String(timeinfo.tm_sec) + String("]");

        int maxPower = request->getParam("maxPower", true)->value().toInt();
        String deviceSn = request->getParam("deviceSn", true)->value();
        auto inverter = SmartStrategy.getInverterByDeviceSn(deviceSn);
        inverter->setInvertPower(maxPower, true);
        root["code"] = 0;
        root["beginTime"] = timeLog;
        getLocalTime(&timeinfo, 5);
        String timeLog2 = String("[") + String(timeinfo.tm_year + 1900) + String("-")
            + String(timeinfo.tm_mon + 1)
            + String("-")
            + String(timeinfo.tm_mday)
            + String(" ")
            + String(timeinfo.tm_hour)
            + String(":")
            + String(timeinfo.tm_min)
            + String(":")
            + String(timeinfo.tm_sec) + String("]");
        root["endTime"] = timeLog2;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
    } catch (...) {
        root["msg"] = String("server fail");
        root["code"] = -1;
    }
}

void XmWebApiV2::smartStrategy(AsyncWebServerRequest* request)
{
    esp_task_wdt_init(30, true);

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    try {
        if (!request->hasParam("smartStrategy", true)) {
            throw CustomException("smartStrategy param not found.", API_PARAMS_NOT_FOUND);
        }
        bool smartStrategy = CommonUtils::isTrue(request, "smartStrategy");
        int inverterCount = SmartStrategy.getInverterCount();
        if (smartStrategy && inverterCount == 0) {
            throw CustomException("at least one inverter is needed.", INVERTER_LIMIT_TWO);
        }
        if (smartStrategy && Shelly.open == false) {
            throw CustomException("shelly is not open.", METER_NOT_OPEN);
        }
        int setInterval = 3U;
        if(!smartStrategy){
            SmartStrategy.setOpen(smartStrategy);
        }
        auto inverters = SmartStrategy.inverters;
        for (int i = 0; i < XM_INVERTER_MAX; i++) {
            if (inverters[i] != nullptr && inverters[i]->getDeviceSn() != "") {
                inverters[i]->openSmartStrategy(smartStrategy);
                int needSetPower = smartStrategy ? 0 : inverters[i]->getMaxPower();
                MessageOutput.println(String("smartStrategy")+String(smartStrategy)+String(",needSetPower")+String(needSetPower));
                try{
                    inverters[i]->setInvertPower(needSetPower,true);
                }catch(CustomException& e){

                }
                if (inverters[i]->getType() == INVERTER_TYPE_DEYE) {
                    // 德业5s一次智能策略扫描
                    setInterval = 3U;
                }
            }
        }
        auto guard = Configuration.getWriteGuard();
        CONFIG_T& config = guard.getConfig();
        config.Xm.SmartStrategy = smartStrategy;
        config.Xm.XmInterval = setInterval;
        if (!Configuration.write()) {
            throw CustomException("dtu config write wrong.", DTU_CONF_WRITE_ERROR);
        }
        SmartStrategy.setOpen(config.Xm.SmartStrategy);
        SmartStrategy.setInterval(config.Xm.XmInterval);
        root["code"] = 0;
    } catch (CustomException& e) {
        root["msg"] = e.msg();
        root["code"] = -1;
    } catch (...) {
        root["msg"] = String("server fail");
        root["code"] = -1;
    }
    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}
