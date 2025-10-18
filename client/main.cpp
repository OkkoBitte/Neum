#include <includelib.hpp>
#include "center.hpp"


class myApp : public clientManager {
public:
    myApp(client_configure& conf) : clientManager(conf), host(std::make_unique<hostManager>(this)) {}
    
    void connected() override {
        std::string text= "hello im client";
        std::vector<uint8_t> data(text.begin(), text.end());
        std::cout<<"SEND:"<<text<<std::endl;
        sendData(data);
        
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
    void closeConnection()override{
        std::cout<<"CLOSE"<<std::endl;

    }
private:
    std::unique_ptr<hostManager> host;
};

int main(int argc, char* argv[]) {
    client_configure cf;
    if(argc > 2){
        cf.hostname = argv[1];
        cf.port = atoi(argv[2]);
    }
    else{
        cf.hostname = "localhost";
        cf.port = 3333;
    }
    
    

    uint8_t opts[10] = {};
    opts[0] = 0x01; // new
    opts[1] = 0x01; // get-close
    opts[2] = 0x0A; // 256
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
