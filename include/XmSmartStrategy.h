#pragma once

#include "Configuration.h"
#include "IXmInverter.h"
#include "MessageOutput.h"
#include <AsyncJson.h>
#include <TaskSchedulerDeclarations.h>

class XmSmartStrategy {
private:
    Task _deyeReadCommnandSendTask;

    Task _deyeReadCommandReceiveTask;

    Task _smartStrategyTask;

    Task _deyeAticvePowerReadTask;

    void deyeCommandRead();
    void deyeCommandWrite();
    void deyeActivePowerRead();

    void smartStrategyTask();

    bool open;

public:
    XmSmartStrategy();

    void init(Scheduler& scheduler);

    void addInverter(IXmInverter* inverter);

    void addInverter(IXmInverter* inverter, int index);

    void removeInverter(String deviceSn);

    void removeInverter(int index);

    void loadByConfig();

    void setOpen(bool open);

    bool getOpen();

    void setInterval(int interval);

    int getInterval();

    int getInverterCount();

    void close();

    void setInverterMaxPower(bool isOpen);

    IXmInverter* getInverterByDeviceSn(String deviceSn);

    IXmInverter* inverters[XM_INVERTER_MAX];

    int invertCount = 0;

    int maxPower = 0;

    int interval = 2;


    int shellyUpdateCloseDataTimes = 0;

    int lastShellyReportPower = 0;


    int arr[3] = {2,5,5};


};

extern XmSmartStrategy SmartStrategy;
