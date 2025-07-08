
#include "XmShelly.h"
#include <AsyncJson.h>
#include <HTTPClient.h>
#include "MessageOutput.h"
#include "__compiled_constants.h"
#include "NetworkSettings.h"
#include "XmException.h"
#include "HTTPClient.h"

void XmShelly::init(CONFIG_T& config,String deviceSn,bool isPro){
    MessageOutput.println(deviceSn);
    this->deviceSn = deviceSn;
    this->isPro = isPro;
    openMDNS(config);
    initShellyHostName(config);
}

void XmShelly::openMDNS(CONFIG_T& config){
    if(config.Mdns.Enabled){
        return;
    }
    MDNS.end();
    MessageOutput.print("begin OpenMDNS");
    if (MDNS.begin(NetworkSettings.getHostname())) {
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("opendtu", "tcp", 80);
        MDNS.addServiceTxt("opendtu", "tcp", "git_hash",__COMPILED_GIT_HASH__);
        config.Mdns.Enabled = true;
    } else {
        throw CustomException("MDNS open ERROR.",DTU_MDNS_OPEN_ERROR);
    }
}

void XmShelly::reload(){
    auto& config =Configuration.get();
    deviceSn = String(config.Xm.Shelly.DeviceSn);
    isPro = config.Xm.Shelly.IsPro;
    hostName = String(config.Xm.Shelly.HostName);
    open = config.Xm.Shelly.open;
}

void XmShelly::initShellyHostName(CONFIG_T& config){
    int n = MDNS.queryService("http","tcp");
    String errorMsg = "mdns found serveice number: " + n;
    MessageOutput.println(String("mdns found service(http,tcp) number:") + String(n));
    if(n > 0){
        for(int i =0;i<n;i++){
            String MDNShostName = MDNS.hostname(i);
            IPAddress ip = MDNS.IP(i);
            MessageOutput.println(String("HOSTNAME:") + MDNShostName +String(",IP:")+ip.toString());
            MDNShostName.toLowerCase();
            if(MDNShostName.indexOf(deviceSn) > 0){
                MDNShostName = String("http://")+MDNShostName + String(".local");
                if(!isPro){
                    hostName = String("http://") + ip.toString();
                }else{
                    hostName = MDNShostName;
                }
                strlcpy(config.Xm.Shelly.HostName, hostName.c_str(),sizeof(config.Xm.Shelly.HostName));
            }
        }
        if(hostName.length() == 0){
            throw CustomException(errorMsg,SHELLY_MDNS_NOT_FOUND_ERROR);
        }
    }else{
            throw CustomException(errorMsg,SHELLY_MDNS_NOT_FOUND_ERROR);
    }
}

int XmShelly::getTotalPower(){
    String serverPath = hostName + String("/status");
    if(isPro){
        serverPath = hostName +  String("/rpc/EM.GetStatus?id=0");
    }
    //serverPath = "http://shellyem3-485519d9dcbf.local/status";
    // MessageOutput.println(String("serverPath:") + serverPath);
    HTTPClient _httpClient;
    _httpClient.begin(serverPath.c_str());
    _httpClient.setTimeout(3000);
    int httpResponseCode = _httpClient.GET();
    // MessageOutput.println(String("responseCode:") + String(httpResponseCode));
    if(httpResponseCode > 0){
        isOnline = true;
        String payload = _httpClient.getString();
        JsonDocument doc;
        char buf[payload.length() + 1];
        strcpy(buf,payload.c_str());
        deserializeJson(doc, buf);
        // MessageOutput.print(String("getTotalPower payload:") + payload);
        if(!doc.containsKey("total_act_power") && !doc.containsKey("total_power")){
            throw CustomException(payload,SHELLY_API_REQUEST_ERROR);
        }
        int maxOutPower = 0;
        if(isPro){
            maxOutPower = doc["total_act_power"];
        }else{
            maxOutPower = doc["total_power"];
        }
        return maxOutPower;
    }else{
        isOnline = false;
        String errorMsg = String("requestUrl:")+ serverPath + String("response Code: ") + httpResponseCode;
        throw CustomException(errorMsg,SHELLY_API_CODE_ERROR);
    }
}

XmShelly Shelly;
