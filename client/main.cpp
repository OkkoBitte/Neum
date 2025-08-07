#include <includelib.hpp>
#include "center.hpp"


class myApp : public clientManager {
public:
    myApp(client_configure& conf) : clientManager(conf), host(std::make_unique<hostManager>(this)) {}
    
    void connected() override {
        std::cout << "Connected successfully!" << std::endl;
        
        for (int t = 0; t < 10; t++) {
            std::cout << "Enter message (" << (t+1) << "/10): ";
            std::string text;
            std::getline(std::cin, text);
            std::vector<uint8_t> fdata(text.begin(), text.end());
            sendData(fdata);
        }
    }
    
    void getData(std::vector<uint8_t> data) override {
        std::cout << "Received data: ";
        for (auto d : data) {
            std::cout << static_cast<char>(d);
        }
        std::cout << std::endl;
    }

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

    try {
        myApp app(cf); 
        
      
      app.loop();
         
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
