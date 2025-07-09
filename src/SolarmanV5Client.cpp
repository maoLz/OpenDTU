#include<WiFiClient.h>
#include "MessageOutput.h"
#include "SolarmanV5Client.h"

SolarmanV5Client::SolarmanV5Client(){
    port = 8899;
}

void SolarmanV5Client::init(String address,int port,String collectorSn){
    this->address = address;
    this->port = port;
    this->collectorSn = collectorSn;
}

void send(String modbusRequest,int sequenceNum){


}
