#include "XmMessage.h"

XmMessageClass::XmMessageClass(){}

void XmMessageClass::println(char* message){
    MqttSettings.publish("dtu/message",String(message));
}

void XmMessageClass::printStr(String message){
    MqttSettings.publish("dtu/message",message);
}

XmMessageClass XmMessage;