#pragma once
#include <exception>
#include <Arduino.h>
using namespace std;

enum ExceptionType{

    NORMAL_RST = 9001,

    UNEXPECT_ERROR = 9999,

    PARAMS_NOT_FOUND = 9002,

    INVERTER_LIMIT_TWO = 9003,

    INVERTER_ALREADY_EXISTS = 9004,

    INVERTER_DEYE_ADD_ERROR=9005,

    METER_NOT_OPEN = 9006,

    API_PARAMS_NOT_FOUND = 1101,

    //MQTT EXCEPTION
    MQTT_MSG_FORMAT_ERROR = 1001,
    MQTT_UNKNOWN_MSG =1002,

    //APS EXCEPTION
    APS_IP_ERROR =2001,
    APS_REQUEST_ERROR = 2002,
    APS_DEVICE_SN_GET_ERROR = 2003,
    APS_POWER_GET_ERROR = 2004,
    APS_POWER_SET_ERROR = 2005,

    //APS_WEB_EXCEPTION
    API_APS_PARAMS_NOT_FOUND = 2101,
    API_APS_IP_FORMAT_ERROR = 2102,

    SHELLY_IS_PRO_NOT_FOUND = 3001,
    SHELLY_DEVICE_SN_NONE_SET = 3002,
    SHELLY_API_REQUEST_ERROR = 3003,
    SHELLY_API_CODE_ERROR = 3004,
    SHELLY_MDNS_NOT_FOUND_ERROR = 3005,

    API_SHELLY_PARARMS_NOT_FOUND = 3101,



    //DTU
    DTU_CONF_WRITE_ERROR = 4001,
    DTU_MDNS_OPEN_ERROR = 4002,
    DTU_START_SMART_ERROR = 4003
};


class CustomException : public exception{
    public:
        CustomException() : message("Error."){};
        CustomException(String str) :message(str){};
        CustomException(String str,ExceptionType type){
            message = str;
            exceptionCode = type;
        }
        const String msg() const throw () {
            return message;
        }
        ExceptionType wrong(){
            return exceptionCode;
        }


    private:
        String message;
        ExceptionType exceptionCode;
};

