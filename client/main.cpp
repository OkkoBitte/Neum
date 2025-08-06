#include <includelib.hpp>
#include "center.hpp"

class myApp : public clientManager {
public:
    myApp(client_configure& conf) : clientManager(conf), host(std::make_unique<hostManager>(this)) {}

    void getData(std::vector<uint8_t> data){
        std::cout<<"get data:";
        for (auto d:data){
            std::cout<<std::hex<<d;
        }
        std::cout<<";"<<std::endl;
        sendData(data); 
    };
    private:
    std::unique_ptr<hostManager> host;
};

int main() {

    client_configure cf;
    
    cf.hostname = "localhost";
    cf.port = 3334;


    uint8_t opts[10] = {};
    opts[0] = 0x01; // new
    opts[2] = 0xFF; // 256
    cf.options = req10_t(opts);

    char msey[20] = "LoreLoracrateEinzAd";
    memcpy(cf.sey.sey_main, msey, sizeof(msey));  
    myApp app(cf); 
    return 0;
}