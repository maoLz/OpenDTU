
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
    for (int i = 0; i < XM_INVERTER_MAX; i++) {
        if (inverters[i] != nullptr && inverters[i]->getDeviceSn() != "") {
            if (inverters[i]->getType() == INVERTER_TYPE_DEYE) {
                DeyeInverter* deyeInverter = (DeyeInverter*)inverters[i];
                deyeInverter->sendCommand("AT+INVDATA=8,0103003500019404\n", false);
            }
        }
    }
}

void XmSmartStrategy::deyeActivePowerRead()
{
    for (int i = 0; i < XM_INVERTER_MAX; i++) {
        if (inverters[i] != nullptr && inverters[i]->getDeviceSn() != "") {
            if (inverters[i]->getType() == INVERTER_TYPE_DEYE) {
                DeyeInverter* deyeInverter = (DeyeInverter*)inverters[i];
                deyeInverter->sendActiveCommand("AT+INVDATA=8,010300560001641A\n", false);
            }
        }
    }
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
    int invertersCount = getInverterCount();
    _smartStrategyTask.setInterval(interval * TASK_SECOND);
    if (!open || invertersCount == 0) {
        _smartStrategyTask.forceNextIteration();
        return;
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
    String log = String("-------------------智能策略日志开始-----------------\n");
    log = log + String("执行时间:") + timeLog + String("\n");

    int shellyTotalPower = 0;
    try {
        shellyTotalPower = Shelly.getTotalPower();
        log = log + String("Shelly 上报总功率为:") + String(shellyTotalPower);

    } catch (CustomException& e) {
        MessageOutput.println(e.msg());
        return;
    }

    int outPutPower = 0;
    for (int i = 0; i < XM_INVERTER_MAX; i++) {
        if (inverters[i] != nullptr && inverters[i]->isOpen()) {
            try {
                String deviceSn = inverters[i]->getDeviceSn();
                log = log + String("\n逆变器sn:") + String(deviceSn) + String(";输出功率:");
                int curOutputPower = inverters[i]->getOutputPower();
                outPutPower += curOutputPower;
                log = log + String(curOutputPower);
            } catch (CustomException& e) {
                MessageOutput.println(e.msg());
            } catch (...) {
                MessageOutput.println("server fail");
            }
        }
    }

    float needSetPower = shellyTotalPower + outPutPower;
    log = log + String("\n总目标功率:") + String(needSetPower);
    if (needSetPower < 0) {
        needSetPower = 0;
    }
    if (needSetPower > maxPower) {
        needSetPower = maxPower;
    }
    MessageOutput.println(log);

    log = String("");
    for (int i = 0; i < XM_INVERTER_MAX; i++) {
        try {

            if (inverters[i] != nullptr && inverters[i]->isOpen()) {
                String deviceSn = inverters[i]->getDeviceSn();
                String tempLog = String("\n<<<<<<<<<<逆变器sn:") + String(deviceSn) + String("开始下发指令>>>>>>>>>>>\n");
                int invertMaxPower = inverters[i]->getMaxPower();
                float singlePower = needSetPower * invertMaxPower  / maxPower;
                tempLog = tempLog + String("逆变器最大功率为:") + String(invertMaxPower) + String(",")+String("目标功率:")+String(singlePower);
                MessageOutput.println(tempLog);
                inverters[i]->setInvertPower((int)singlePower);
                tempLog = String("\n<<<<<<<<<<逆变器sn:") + String(deviceSn) + String("下发指令结束>>>>>>>>>>>\n");
                MessageOutput.println(tempLog);
            }
        } catch (CustomException& e) {
            MessageOutput.println(e.msg());
        } catch (...) {
            MessageOutput.println("server fail");
        }
    }
    _smartStrategyTask.setInterval(interval * TASK_SECOND);

    getLocalTime(&timeinfo, 5);
    timeLog = String("[") + String(timeinfo.tm_year + 1900) + String("-")
        + String(timeinfo.tm_mon + 1)
        + String("-")
        + String(timeinfo.tm_mday)
        + String(" ")
        + String(timeinfo.tm_hour)
        + String(":")
        + String(timeinfo.tm_min)
        + String(":")
        + String(timeinfo.tm_sec) + String("]");
    log = log + String("\n结束时间:") + timeLog;
    log = log + String("\n-------------------智能策略日志结束-----------------");
    MessageOutput.println(log);
}

/**
 * 添加逆变器 内存
 */
void XmSmartStrategy::addInverter(IXmInverter* inverter)
{
    MessageOutput.println("addInverter Begin!!!");
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
    MessageOutput.println("loadByConfig Begin!!!");
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
    MessageOutput.println("loadByConfig End!!!");
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
