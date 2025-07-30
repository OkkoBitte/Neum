class hostManager;
class serverManager {
private:
    std::map<hex_t, client_connection_data> virtual_connections;
    std::mutex map_mutex;
    std::atomic <bool> work {true};
    client_connection_data* getClient_unsafe(hex_t hexClient) {
        auto it = virtual_connections.find(hexClient);
        return it != virtual_connections.end() ? &it->second : nullptr;
    }

    bool mdfClient_unsafe(hex_t hexClient, const client_connection_data& newData) {
        auto it = virtual_connections.find(hexClient);
        if (it == virtual_connections.end()) {
            return false;
        }
        it->second = newData;
        return true;
    }

    bool purgeClient_unsafe(hex_t hexClient) {
    try {
        
        
        auto it = virtual_connections.find(hexClient);
        if (it == virtual_connections.end()) {
            log::warn ( "Client not found");
            return false;
        }

      
        if (it->second.desc != -1) {
            const int fd = it->second.desc;
            it->second.desc = -1; 
            close(fd);
        }

        virtual_connections.erase(it++);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error in purge: " << e.what() << std::endl;
        return false;
    }
}
    bool dltClient_unsafe(hex_t hexClient){
        auto it = virtual_connections.find(hexClient);
        if (it == virtual_connections.end()) return false;
        
        virtual_connections.erase(it);
        return true;
    }

protected:

    PacketController::packetManager pmanager;
    SeyController::seyManager smanager;
    std::unique_ptr<hostManager> hmanager;


public:

    server_configure serverConfigure;
    virtual void newClientConnection(hex_t client_hex) = 0;
    virtual void getClientData(hex_t client_hex, const std::vector<uint8_t>& data) = 0;
    
    serverManager(server_configure config): serverConfigure(config), pmanager(), smanager(config), work(true){
        hmanager = std::make_unique<hostManager>(*this);
        log::def("Managmant start");
        std::thread managerThread([&]() { MENEGMANT_CLIENTS(); });
        managerThread.detach();

     

    }

    bool addClient(const client_connection_data& newData) {
        std::lock_guard<std::mutex> lock(map_mutex);
        auto hex = hex_t(newData);
        return virtual_connections.insert_or_assign(hex, newData).second;
        
    }
    
    client_connection_data* getClient(hex_t hexClient) {
        std::lock_guard<std::mutex> lock(map_mutex);
        if (auto it = virtual_connections.find(hexClient); it != virtual_connections.end()) {
            return &it->second;
        }
        return nullptr;
    }

    bool mdfClient(hex_t hexClient, const client_connection_data& newData) {
        std::lock_guard<std::mutex> lock(map_mutex);
        return mdfClient_unsafe(hexClient, newData);
    }

    bool dltClient(hex_t hexClient) {
        std::lock_guard<std::mutex> lock(map_mutex);
        return dltClient_unsafe(hexClient);
    }

    bool purgeClient(hex_t hexClient) {
        std::lock_guard<std::mutex> lock(map_mutex);
        return purgeClient_unsafe(hexClient);
    }

    size_t getClientConnectionsCount(){
        std::lock_guard<std::mutex> lock(map_mutex);
        return virtual_connections.size();
    }

    void MENEGMANT_CLIENTS() {
        int when = 0;
        while (work) {
            if ((when % 5) != 0) {
                std::lock_guard<std::mutex> lock(map_mutex);
                
              
                std::vector<hex_t> to_remove;
                for (const auto& [hex, data] : virtual_connections) {
                    req10_t co = data.client_options;
                    time_t now = time(nullptr);

                    if ((now - data.last_activity_time) > serverConfigure.sleepClients) {
                        if (co.code3 == 0x00 || 
                            ((now - data.last_activity_time) > int(co.code3) && 
                            (int(co.code3) > serverConfigure.maxSleepClients))) {
                            to_remove.push_back(hex);
                        }
                    }
                }
            
                for (const auto& hex : to_remove) {
                    purgeClient_unsafe(hex);
                }
            }
            else {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            when = (when > 9999) ? 0 : when + 1;
        }
    }

    void stop();
    void loop(){ while (work){ std::this_thread::sleep_for(std::chrono::seconds(1));} }
};


class hostManager{
    private:
    serverManager& manager;
    int socket_desc;
    protected:
    std::atomic<bool> isActive{false};
    public:
    
    hostManager(serverManager& mgr) : manager(mgr), socket_desc(-1), isActive(false) {
        std::thread socketServerThread( [&] () {startSocketServer();});
        socketServerThread.detach();
    }

    
    bool startSocketServer() {
        if(isActive) return false;
        socket_desc = socket(AF_INET, SOCK_STREAM, 0);
        if(socket_desc == STATUS_SYSTEM_OPERATION_ERROR) {
            log::err("Fatal error, not opening socket");
            std::exit(STATUS_OPERATION_ERROR);
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(manager.serverConfigure.port);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if(bind(socket_desc, (sockaddr*)&serverAddr, sizeof(serverAddr)) == STATUS_SYSTEM_OPERATION_ERROR) {
            log::err("Fatal error, not binding socket");
            close(socket_desc);
            std::exit(STATUS_OPERATION_ERROR);
        }

        if(listen(socket_desc, manager.serverConfigure.maxClients) == STATUS_SYSTEM_OPERATION_ERROR) {
            log::err("Fatal error, not listening socket");
            close(socket_desc);
            std::exit(STATUS_OPERATION_ERROR);
        }

        log::def("server start, port " + std::to_string(manager.serverConfigure.port));
        isActive = true;

        while(isActive) {

            sockaddr_in clientAddr;
            socklen_t clientAddrSize = sizeof(clientAddr);
            int clientSocket = (manager.getClientConnectionsCount() < manager.serverConfigure.maxClients)? accept(socket_desc, (sockaddr*)&clientAddr, &clientAddrSize) : STATUS_SYSTEM_OPERATION_ERROR;
   
            if(clientSocket == STATUS_SYSTEM_OPERATION_ERROR) {
                log::warn("Error accepting connection");
                continue;
            }
  
            time_t now = time(nullptr);
            client_head_packet_raw header;
            
          
            if(recv(clientSocket, &header, sizeof(header), MSG_WAITALL) != sizeof(header)) {
                log::warn("Failed to read client header");
                close(clientSocket);
                continue;
            }

          
            char session_key[20] = {0};
            strncpy(session_key, header.client_sey, sizeof(header.client_sey));
            
            client_connection_data new_client(
                now,
                req10_t(header.client_options), 
                sey_t(session_key), 
                clientSocket,
                clientAddr
            );
            hex_t clien_hex = hex_t(new_client);
                
            if(!manager.addClient(new_client)) {
                log::warn("Failed to add client to manager");
                close(clientSocket);
                continue;
            }
            manager.newClientConnection(clien_hex);
            std::thread([this, clien_hex]() {
                this->handleClient(clien_hex);
            }).detach();

            
        } log::def("stoped socket server");
        return true;
    }
    void handleClient(hex_t client_hex) {
        try {
            auto ccd = manager.getClient(client_hex);   
            if(!ccd) return; 
            ccd->last_activity_time = time(nullptr);
            manager.addClient(*ccd);
            
            while (isActive){
                packet_s packet;
                if(recv(ccd->desc, &packet, sizeof(packet), MSG_WAITALL) != sizeof(packet)) {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    continue;
                }
    
                    
                uint16_t size = (packet.datasize[1] << 8) | packet.datasize[0];
                std::vector<uint8_t> data(size);
                if(recv(ccd->desc, data.data(), size, MSG_WAITALL) != size) {
                    log::warn("Failed to read packet data");
                    continue;
                }

                manager.getClientData(client_hex, data);
            }
            
            //manager.pmanager....
            //manager.getClientData();
            
            // ... next ...

        } catch(const std::exception& e) {
            log::err(std::string("Client error: ") + e.what());
            
        }
    }
    bool reStartSocketServer(){
        stopSocketServer();
        startSocketServer();
        return true;
    }
    bool stopSocketServer(){
        std::string wtxt = "stoping host in port "+std::to_string(manager.serverConfigure.port);
        log::warn(wtxt);
        if(isActive) {
            isActive = false;
            return close(socket_desc);
        }
        return false;
    }
};

void serverManager::stop() { 
    work = false;
    log::warn("stoping server Manager");
    if(hmanager) hmanager->stopSocketServer();
    else log::err("not initializen H-manager");
}

