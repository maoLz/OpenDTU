#pragma once

#include "Configuration.h"
#include <AsyncJson.h>

class IXmInverter{

public:
    /**
     * 逆变器设备初始化
     * 只往配置中写入
     */
    virtual void init(AsyncWebServerRequest* request, CONFIG_T& config, JsonVariant& doc);

    virtual void remove(AsyncWebServerRequest* request, CONFIG_T& config,JsonVariant& doc);


    /**
     * 从配置加载设备
     */
    virtual void loadByConfig()=0;

    /**
     * 逆变器是否开启
     */
    virtual bool isOpen()=0;

    /**
     * 获取逆变器设备序列号
     */
    virtual String getDeviceSn()=0;

    /**
     * 设置逆变器功率
     */
    virtual void setInvertPower(int power)=0;


    virtual void setInvertPower(int power,bool isWebRequest)=0;

    /**
     * 获取逆变器设置功率
     */
    virtual int getInvertPower()=0;


    /**
     * 获取逆变器设置功率
     */
    virtual int getCommandPower()=0;


    /**
     * 获取逆变器输出功率
     */
    virtual float getOutputPower()=0;

    /**
     * 获取逆变器最大功率
     */
    virtual int getMaxPower()=0;

    virtual int getRatedPower()=0;


    virtual void openSmartStrategy(bool smartStrategy)=0;



    virtual int getType()=0;

    virtual int getIndex()=0;

    virtual String getRemark()=0;
};
