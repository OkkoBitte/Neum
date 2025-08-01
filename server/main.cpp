#include <includelib.hpp>
#include "center.cpp"

std::string confpathp;

class MyServerManager : public serverManager {
public:
    MyServerManager() : serverManager(serverConfigureController::getConfigFropPath(confpathp)) {
        
    }
    void newClientConnection(hex_t client_hex) override {
        auto client = getClient(client_hex);
        std::cout<<"CONNECTED: "<< client->sey.sey_main <<" ["<<client->desc<< "]" <<std::endl;
    }

    void getClientData(hex_t client_hex, const std::vector<uint8_t>& data) override {
        std::cout<<"get:";
        for(auto bite : data){
            std::cout<<std::hex<<bite;
        }
        std::cout<<std::endl;
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
            confpathp = bedl;
            MyServerManager myapp;
    

            myapp.loop();
           
            
        }

    }
}

