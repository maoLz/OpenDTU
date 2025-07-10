
#include "XmSmartStrategy.h"
#include "ApsInverter.h"
#include "DeyeInverter.h"
#include "XmException.h"
#include "XmShelly.h"
#include <cstdlib>

XmSmartStrategy::XmSmartStrategy()
    : _deyeReadCommandReceiveTask(3 * TASK_SECOND, TASK_FOREVER, std::bind(&XmSmartStrategy::deyeCommandWrite, this))
    , _deyeReadCommnandSendTask(3 * TASK_SECOND, TASK_FOREVER, std::bind(&XmSmartStrategy::deyeCommandRead, this))
    , _deyeAticvePowerReadTask(10 * TASK_SECOND, TASK_FOREVER, std::bind(&XmSmartStrategy::deyeActivePowerRead, this))
    , _smartStrategyTask(5 * TASK_SECOND, TASK_FOREVER, std::bind(&XmSmartStrategy::smartStrategyTask, this))
{
}
/**
 * 智能策略 初始化
 */
void XmSmartStrategy::init(Scheduler& scheduler)
{
    // scheduler.addTask(_deyeReadCommandReceiveTask);
    // scheduler.addTask(_deyeReadCommnandSendTask);
    // scheduler.addTask(_deyeAticvePowerReadTask);
    scheduler.addTask(_smartStrategyTask);
    //_deyeReadCommandReceiveTask.enable();
    // _deyeReadCommnandSendTask.enable();
    //_deyeAticvePowerReadTask.enable();
    _smartStrategyTask.enable();
    Shelly.reload();
    loadByConfig();
}

/**
 * 德业逆变器读取功率指令 发出任务
 */
void XmSmartStrategy::deyeCommandWrite()
{

}

void XmSmartStrategy::deyeActivePowerRead()
{
}

/**
 * 德业逆变器读取功率指令 接收任务(需要异步处理)
 */
void XmSmartStrategy::deyeCommandRead()
{

}

/**
 * 智能策略 任务
 */
void XmSmartStrategy::smartStrategyTask()
{
    _smartStrategyTask.setInterval(interval * TASK_SECOND);

    int invertersCount = getInverterCount();
    if (!open || invertersCount == 0) {
        _smartStrategyTask.forceNextIteration();
        return;
    }
    char log[256];
    int startHeap = ESP.getFreeHeap();
    snprintf(log,sizeof(log),"***smart strategy begin***\n[Memory] Free heap:%d bytes.\n",startHeap);
    MessageOutput.println(log);
    int shellyTotalPower = 0;
    try {
        shellyTotalPower = Shelly.getTotalPower();
        snprintf(log,sizeof(log),"shelly TotalPower:%d.\n",shellyTotalPower);
        MessageOutput.println(log);
    } catch (CustomException& e) {
        snprintf(log,sizeof(log),"shelly getTotalPower wrong.\n");
        MessageOutput.println(log);
        return;
    }
    int outPutPower = 0;
    for (int i = 0; i < XM_INVERTER_MAX; i++) {
        if (inverters[i] != nullptr && inverters[i]->isOpen()) {
            String deviceSn = inverters[i]->getDeviceSn();
            try {
                int curOutputPower = inverters[i]->getOutputPower();
                snprintf(log,size(log),"[%s] current output Power:%d.\n",deviceSn.c_str(),curOutputPower);
                MessageOutput.println(log);
                outPutPower += curOutputPower;
            } catch (...) {
                snprintf(log,size(log),"[%s] current output Power get wrong.\n",deviceSn.c_str());
                MessageOutput.println(log);
            }
        }
    }

    float needSetPower = shellyTotalPower + outPutPower;
    if (needSetPower < 0) {
        needSetPower = 0;
    }
    if (needSetPower > maxPower) {
        needSetPower = maxPower;
    }
    snprintf(log,size(log),"all inverter's outputPower: %d,totalTargetPower(shellyTotalPower+outPutPower):%f.\n",outPutPower,needSetPower);
    MessageOutput.println(log);
    for (int i = 0; i < XM_INVERTER_MAX; i++) {
        try {
            if (inverters[i] != nullptr && inverters[i]->isOpen()) {
                int invertMaxPower = inverters[i]->getMaxPower();
                float singlePower = needSetPower * invertMaxPower  / maxPower;
                snprintf(log,size(log),"[%s] target power: %f\n",inverters[i]->getDeviceSn().c_str(),singlePower);
                MessageOutput.print(log);
                inverters[i]->setInvertPower((int)singlePower);
            }
        } catch (CustomException& e) {
        } catch (...) {
        }
    }
    MessageOutput.print("***smart strategy end***\n");;
    _smartStrategyTask.setInterval(interval * TASK_SECOND);


}

/**
 * 添加逆变器 内存
 */
void XmSmartStrategy::addInverter(IXmInverter* inverter)
{

    for (int i = 0; i < XM_INVERTER_MAX; i++) {
        if (inverters[i] == nullptr || inverters[i]->isOpen() == false) {
            inverters[i] = inverter;
            invertCount++;
            maxPower += inverter->getMaxPower();
            break;
        }
    }
}

void XmSmartStrategy::addInverter(IXmInverter* inverter, int index)
{
    if (index >= 0 && index < XM_INVERTER_MAX) {
        inverters[index] = inverter;
        invertCount++;
        maxPower += inverter->getMaxPower();
    }
}

void XmSmartStrategy::removeInverter(String deviceSn)
{
    for (int i = 0; i < XM_INVERTER_MAX; i++) {
        if (inverters[i] != nullptr && inverters[i]->getDeviceSn() == deviceSn) {
            maxPower -= inverters[i]->getMaxPower();
            inverters[i] = nullptr;
            invertCount--;
            break;
        }
    }
}

void XmSmartStrategy::removeInverter(int index)
{
    if (index >= 0 && index < XM_INVERTER_MAX) {
        if (inverters[index] != nullptr) {
            maxPower -= inverters[index]->getMaxPower();
            inverters[index] = nullptr;
            invertCount--;
        }
    }
}

void XmSmartStrategy::loadByConfig()
{

    auto& config = Configuration.get();
    open = config.Xm.SmartStrategy;
    interval = config.Xm.XmInterval;
    int invertNumber = 0;
    int maxPowerNumber = 0;
    for (int i = 0; i < XM_INVERTER_MAX; i++) {
        XM_INVERTER_T inverter = config.Xm.Inverters[i];
        if (inverter.Open) {
            invertNumber++;
            maxPowerNumber += inverter.MaxPower;
            if (inverter.Type == INVERTER_TYPE_DEYE) {
                addInverter(new DeyeInverter(inverter), i);
            } else if (inverter.Type == INVERTER_TYPE_APS) {
                addInverter(new ApsInverter(inverter), i);
            }
        } else {
            removeInverter(i);
        }
    }
    invertCount = invertNumber;
    this->maxPower = maxPowerNumber;

}

void XmSmartStrategy::setOpen(bool open)
{
    this->open = open;
}

void XmSmartStrategy::setInverterMaxPower(bool isOpen)
{
    for (int i = 0; i < XM_INVERTER_MAX; i++) {
        if (inverters[i] != nullptr && inverters[i]->getDeviceSn() != "") {
            int needSetPower = inverters[i]->getMaxPower();
            try {
                if(isOpen || inverters[i]->getType() == INVERTER_TYPE_APS){
                    inverters[i]->setInvertPower(needSetPower, true);
                }
            } catch (CustomException& e) {
            }
        }
    }
}

bool XmSmartStrategy::getOpen()
{
    return this->open;
}

void XmSmartStrategy::setInterval(int interval)
{
    this->interval = interval;
}

int XmSmartStrategy::getInterval()
{
    return this->interval;
}

int XmSmartStrategy::getInverterCount()
{
    return invertCount;
}

IXmInverter* XmSmartStrategy::getInverterByDeviceSn(String deviceSn)
{
    for (int i = 0; i < XM_INVERTER_MAX; i++) {
        if (inverters[i] != nullptr && inverters[i]->getDeviceSn() == deviceSn) {
            return inverters[i];
        }
    }
    return nullptr;
}

XmSmartStrategy SmartStrategy;
