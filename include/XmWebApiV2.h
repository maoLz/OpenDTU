#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class XmWebApiV2 {
public:
    void init(AsyncWebServer& server);
private:
    void onAddInverter(AsyncWebServerRequest* request);

    void onRemoveInverter(AsyncWebServerRequest* request);


    void onLoadConfig(AsyncWebServerRequest* request);

    void smartStrategy(AsyncWebServerRequest* request);

    void setMaxPower(AsyncWebServerRequest* request);

    void spaceDataForDev(AsyncWebServerRequest* request);

};
