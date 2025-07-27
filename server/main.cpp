#include <includelib.hpp>
#include "center.hpp"



class MyHost : public serverManager::Host {
public:
    serverManager &manager;

    MyHost(serverManager& mgr) : serverManager::Host(mgr), manager(mgr) {}
    
    void processClientData(hex_t client_hex, const std::vector<uint8_t>& data) override {
        
        std::string message(data.begin(), data.end());
        
        client_connection_data clientdata = manager.getClient(client_hex);
        
        std::cout << "Received from client " << clientdata.sey.sey_main << ": " << message << std::endl;

        std::string response = "Server received your message";
        
    }
};

int main(int argc, char* argv[]) {  

    if (argc == 1){
        std::cout << "|[Neum:V] "<<argv[0]<<" <filename> or --help"<<std::endl;
    }
    else if(argc > 1){
        std::string bedl = std::string (argv[1]);

        if(bedl == "--help"){
            std::cout<<help1_server;
        }
        else if(bedl == "--describe"){
            std::cout<<help2_server;
        }
        else{
            serverManager sm(serverConfigureController::getConfigFropPath(bedl));
            MyHost host(sm);
            
            std::thread serverThread([&host]() { host.startSocketServer(); });
            std::thread managerThread([&sm]() { sm.MENEGMANT_CLIENTS(); });
            serverThread.detach();
            managerThread.join();

        }

    }
}