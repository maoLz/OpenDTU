#include "MqttSettings.h"

class XmMessageClass{

public:
    XmMessageClass();
    void println(char* message);
    void printStr(String message);
};

extern XmMessageClass XmMessage;