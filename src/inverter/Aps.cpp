#include "ApsInverter.h"
#include "CommonUtils.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "XmException.h"
#include "XmSmartStrategy.h"
#include <AsyncJson.h>
#include <HTTPClient.h>

ApsInverter::ApsInverter()
{
}

ApsInverter::ApsInverter(XM_INVERTER_T& inverter)
{
    _deviceSn = String(inverter.DeviceSn);
    _ip = IPAddress(inverter.Ip).toString();
    _maxPower = inverter.MaxPower;
    _isOpen = inverter.Open;
    _hostName = String("http://") + _ip + String(":8050");
    _type = inverter.Type;
    _index = inverter.Index;
}

void ApsInverter::init(AsyncWebServerRequest* request, CONFIG_T& config, JsonVariant& doc)
{
    doc["code"] = 0;
    try {
        String spaceInverterSn = request->getParam("spaceInverterSn", true)->value();
        Configuration.clearInverterSlotBySpaceDeviceSn(spaceInverterSn);

        XM_INVERTER_T* inverter = Configuration.getFreeXMInverterSlot();
        if (inverter == nullptr) {
            // TODO 逆变器没有空位置
            MessageOutput.println("DTU inverter is out of size.You need Remove one Inverter.");
            throw CustomException("A maximum of two inverters can be added", INVERTER_LIMIT_TWO);
        }
        /**
         * 打开本地模式时，需要配置IP地址。调用APS接口
         */
        if (!request->hasParam("ip", true)) {
            throw CustomException("ip param not found.", PARAMS_NOT_FOUND);
        }
        String ip = request->getParam("ip", true)->value();
        IPAddress ipaddress;
        if (!ipaddress.fromString(ip)) {
            throw CustomException("ip format wrong.", PARAMS_NOT_FOUND);
        }
        /**
         * 将数据写入对象中
         */
        _hostName = String("http://") + ip + String(":8050");
        JsonDocument response;
        requestAPI(APS_GET_DEVICE_INFO, response);
        _deviceSn = String(response["data"]["deviceId"]);
        if (Configuration.getXMInverterByDeviceSn(_deviceSn.c_str()) != nullptr) {
            throw CustomException("Device already exists.", INVERTER_ALREADY_EXISTS);
        }
        _maxPower = response["data"]["maxPower"].as<int>();
        _ip = ip;
        _isOpen = true;
        _type = INVERTER_TYPE_APS;
        /**
         * 将数据写入配置中
         */

        inverter->Ip[0] = ipaddress[0];
        inverter->Ip[1] = ipaddress[1];
        inverter->Ip[2] = ipaddress[2];
        inverter->Ip[3] = ipaddress[3];
        inverter->MaxPower = _maxPower;
        inverter->Open = true;
        inverter->Type = INVERTER_TYPE_APS;
        strlcpy(inverter->DeviceSn, _deviceSn.c_str(), DEVICE_SN_LENGTH);
        /**
         * 将当前逆变器添加至内存
         */
        SmartStrategy.addInverter(this, inverter->Index);
        /**
         * 返回数据
         */
        doc["deviceSn"] = _deviceSn;
        //doc["maxPower"] = _maxPower;
        //doc["ip"] = _ip;
        //doc["type"] = INVERTER_TYPE_APS;
        if (!Configuration.write()) {
            throw CustomException("dtu config write wrong.", DTU_CONF_WRITE_ERROR);
        }
    } catch (CustomException& e) {
        /**
         * 失败时，需要重置配置以及从配置还原内存中的属性
         */
        Configuration.read();
        loadByConfig();
        throw e;
    } catch (...) {
        Configuration.read();
        loadByConfig();
        throw CustomException("server fail");
    }
}

void ApsInverter::remove(AsyncWebServerRequest* request, CONFIG_T& config, JsonVariant& doc)
{

}

void ApsInverter::loadByConfig()
{
    // Load inverter settings based on the configuration
    // Add your loading logic here
}

bool ApsInverter::isOpen()
{
    // Check if the inverter is currently open
    // Return true or false based on the state
    return _isOpen; // Placeholder return value
}

String ApsInverter::getDeviceSn()
{
    // Return the device serial numberP
    return _deviceSn; // Placeholder serial number
}

void ApsInverter::setInvertPower(int power)
{
    // Set the inverter power
    // Add your logic to set the power
    if (power < 30) {
        power = 30;
    } else if (power > _maxPower) {
        power = _maxPower;
    }
    String api = String("setMaxPower?p=") + String(power);
    MessageOutput.println(String("newAPS,setInverterPower")+api);
    JsonDocument doc;
    String payload = requestAPI(api, doc);
    MessageOutput.println(String("deviceSn:")+_deviceSn + String(",指令及结果:")+payload);
    if (!String("SUCCESS").equals(doc["message"].as<String>())) {
        MessageOutput.println("本次智能策略APS下发指令失败\n");
        throw CustomException(payload, APS_POWER_SET_ERROR);
    }
}

void ApsInverter::setInvertPower(int power, bool isWebRequest)
{
    setInvertPower(power);
}

int ApsInverter::getInvertPower()
{
    return (int) getOutputPower();
}

int ApsInverter::getMaxPower()
{
    // Return the maximum power the inverter can handle
    return _maxPower; // Placeholder maximum power
}

int ApsInverter::getCommandPower()
{
    JsonDocument doc;
    String payload = requestAPI("getMaxPower",doc);
    return doc["data"]["power"];
    // Return the maximum power the inverter can handle
}

float ApsInverter::getOutputPower()
{
    JsonDocument doc;
    requestAPI("getOutputData", doc);
    return doc["data"]["p1"].as<float>() + doc["data"]["p2"].as<float>();
}

int ApsInverter::getType()
{
    return _type;
}

int ApsInverter::getIndex()
{
    return _index;
}

String ApsInverter::getRemark()
{
    return String("this is aps inverter");
}

String ApsInverter::requestAPI(String api, JsonDocument& doc)
{
    String apiUrl = _hostName + String("/") + api;
    HTTPClient _httpClient;
    _httpClient.setTimeout(2000);
    _httpClient.begin(apiUrl.c_str());
    int httpResponseCode = _httpClient.GET();
    if (httpResponseCode > 0) {
        String payload = _httpClient.getString();
        char buf[payload.length() + 1];
        strcpy(buf, payload.c_str());
        deserializeJson(doc, buf);
        return payload;
    } else {
        String errorMsg = String("request url:") + apiUrl + String(";response Code") + httpResponseCode;
        throw CustomException(errorMsg, APS_REQUEST_ERROR);
    }
    _httpClient.end();
}

int ApsInverter::getRatedPower(){
    return _maxPower;
}

void ApsInverter::openSmartStrategy(bool smartStrategy){

}
