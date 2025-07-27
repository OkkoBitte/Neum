namespace SeyController{
    class seyManager{
        private:
        std::vector<sey_t> seys;
        std::string pathToSey;
        server_configure serverConfigure;

        void seyDumps(){
            
        }

        public:
        seyManager(server_configure sc): serverConfigure(sc) {
            pathToSey = sc.pathClientsData;
            seyDumps();
        }

        std::optional<sey_t> getSey(sey_t seyf){
            for (auto sey : seys){
                if( sey == seyf ) return sey;
            }
            return std::nullopt;
        }


    };

    bool isSey (SeyController::seyManager& sm, sey_t sey){
        return (sm.getSey(sey)!= std::nullopt)? true : false;
    };       
};

namespace serverConfigureController{
    server_configure getConfigFropPath(std::string path){
        if(!file::is_file(path)){
            std::string errort = "not find file '" + path+"'";
            log::err(errort);
            std::exit(STATUS_OPERATION_ERROR);
        }
        
        std::string file_data = file::read_file<std::string>(path);
        std::vector<vars> kvar = kconf(file_data, {});
        server_configure retconf;

      
        retconf.maxClients = 15;
        retconf.sleepClients = 10;
        retconf.maxSleepClients = 20;
        retconf.maxGetClientPacket = 1000000;
        

        bool hasPort = false;
        bool hasPathClientsData = false;

        for (auto var : kvar){
            if(var.name == "port"){
                if (var.valib.type == VSID::intV){
                    retconf.port = atoi(var.valib.value.c_str());
                    hasPort = true;
                }
                else{
                    log::err("var 'port' need type INT");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            else if (var.name == "maxClients"){
                if (var.valib.type == VSID::intV){
                    retconf.maxClients = atoi(var.valib.value.c_str());
                }
                else{
                    log::err("var 'maxClients' need type INT");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            else if (var.name == "sleepClients"){
                if (var.valib.type == VSID::intV){
                    retconf.sleepClients = atoi(var.valib.value.c_str());
                }
                else{
                    log::err("var 'sleepClients' need type INT");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            else if (var.name == "maxSleepClients"){
                if (var.valib.type == VSID::intV){
                    retconf.maxSleepClients = atoi(var.valib.value.c_str());
                }
                else{
                    log::err("var 'maxSleepClients' need type INT");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            else if (var.name == "maxGetClientPacket"){
                if (var.valib.type == VSID::intV){
                    retconf.maxGetClientPacket = atoi(var.valib.value.c_str());
                }
                else{
                    log::err("var 'maxGetClientPacket' need type INT");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            else if (var.name == "pathClientsData"){
                if (var.valib.type == VSID::stringV){
                    retconf.pathClientsData = var.valib.value;
                    hasPathClientsData = true;
                }
                else{
                    log::err("var 'pathClientsData' need type STRING");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
        }
    
        if(!hasPort) {
            log::err("Configuration error: 'port' is required but not specified");
            std::exit(STATUS_OPERATION_ERROR);
        }
        
        if(!hasPathClientsData) {
            log::err("Configuration error: 'pathClientsData' is required but not specified");
            std::exit(STATUS_OPERATION_ERROR);
        }
        
        return retconf;
    }
}





namespace PacketController{

}