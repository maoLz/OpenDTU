#include <AsyncJson.h>
#include "MessageOutput.h"

class SolarmanV5Client {

    public:
        SolarmanV5Client();
        void init(String address,int port,String collectorSn);
    private:
        String address;
        int port;
        String collectorSn;
};
