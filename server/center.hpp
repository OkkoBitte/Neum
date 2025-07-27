class serverManager {
private:
    std::map<hex_t, client_connection_data> virtual_connections;
    std::mutex map_mutex;
    std::atomic <bool> work {true};
    
    client_connection_data& getClient_unsafe(hex_t hexClient) {
        return virtual_connections.at(hexClient);
    }

    bool mdfClient_unsafe(hex_t hexClient, const client_connection_data& newData) {
       auto result = virtual_connections.insert_or_assign(hexClient, newData);
       return result.second;
    }
    void purgeClient_unsafe(client_connection_data cd){
        close(cd.desc);
        virtual_connections.erase(hex_t(cd));
    }

public:
    class Host{
        private:
        serverManager& manager;
        int socket_desc;
        std::atomic<bool> isActive{false};
        
        public:
        
        Host(serverManager& mgr) : manager(mgr), socket_desc(-1), isActive(false) {}
        
        virtual void processClientData(hex_t client_hex, const std::vector<uint8_t>& data) = 0;
        
        void handleClient(client_connection_data client_data, client_packet_raw packet_raw) {
            try {
                
                uint16_t data_size;
                memcpy(&data_size, packet_raw.data_size, sizeof(data_size));
                data_size = ntohs(data_size);

                
                const size_t MAX_ALLOWED_SIZE = manager.serverConfigure.maxGetClientPacket;
                if(data_size > MAX_ALLOWED_SIZE) {
                    throw std::runtime_error("Data size too large");
                }

               
                std::vector<uint8_t> fulldata(data_size);

                
                size_t total_read = 0;
                while(total_read < data_size) {
                    int bytesRead = recv(client_data.desc, fulldata.data() + total_read, 
                                        data_size - total_read, 0);
                    
                    if(bytesRead <= 0) {
                        throw std::runtime_error("Connection closed prematurely");
                    }
                    total_read += bytesRead;
                }

                
                {
                    client_connection_data& ccd = manager.getClient(hex_t(client_data));
                    ccd.last_activity_time = time(nullptr);
                }

                // ... next ...
                processClientData(hex_t(client_data), fulldata);

            } catch(const std::exception& e) {
                log::err(std::string("Client error: ") + e.what());
                purgeClient(client_data);
            }
        }
        void purgeClient(client_connection_data cd){
            close(cd.desc);
            manager.dltClient(hex_t(cd));
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
                client_packet_raw header;
                
                
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

                
                if(!manager.addClient(new_client)) {
                    log::warn("Failed to add client to manager");
                    close(clientSocket);
                    continue;
                }
                std::thread([this, new_client, packet = header]() {
                    this->handleClient(new_client, packet);
                }).detach();
            }
            stopSocketServer();
            return true;
        }
        bool reStartSocketServer(){
            stopSocketServer();
            startSocketServer();
            return true;
        }
        bool stopSocketServer(){
            if(isActive) {
                isActive = false;
                return close(socket_desc);
            }
            return false;
        }
    };
    server_configure serverConfigure;
    serverManager(server_configure config): serverConfigure(config){

    }
    bool addClient(const client_connection_data& newData) {
        std::lock_guard<std::mutex> lock(map_mutex);
        auto hex = hex_t(newData);
        return virtual_connections.emplace(std::move(hex), newData).second;
    }

    client_connection_data& getClient(hex_t hexClient) {
        std::lock_guard<std::mutex> lock(map_mutex);
        return getClient_unsafe(hexClient);
    }

    bool mdfClient(hex_t hexClient, const client_connection_data& newData) {
        std::lock_guard<std::mutex> lock(map_mutex);
        return mdfClient_unsafe(hexClient, newData);
    }

    bool dltClient(hex_t hexClient) {
        std::lock_guard<std::mutex> lock(map_mutex);
        return virtual_connections.erase(hexClient) > 0;
    }
    void purgeClient(hex_t hexClient){
        client_connection_data & cd = getClient(hexClient);
        close(cd.desc);
        dltClient(hexClient);
    }
    size_t getClientConnectionsCount(){
        std::lock_guard<std::mutex> lock(map_mutex);
        return virtual_connections.size();
    }
    void MENEGMANT_CLIENTS() {
        int when = 0;
        while (work) {

            if ((when%5) != 0){
                std::lock_guard<std::mutex> lock(map_mutex);
        
                for(auto& [hex, data] : virtual_connections) {
                
                    // LINTER WERBLIXTEN
                    req10_t co = data.client_options;
                    time_t now = time(nullptr);
                    if ((now - data.last_activity_time) > serverConfigure.sleepClients){
                        if (co.code3 == 0xC0){ // CLOSE
                            purgeClient_unsafe(data);
                        }
                        else{
                            if ((now - data.last_activity_time) > int(co.code3) && int(co.code3) > serverConfigure.maxSleepClients){ // WAIT CLIENT SECONDS
                                purgeClient_unsafe(data);
                            }
                        }
                    }
                    
                }
            }
            else std::this_thread::sleep_for(std::chrono::seconds(1));

            when++;
            when = (when>9999)? 0 : when;
        }
    }
    void stop(){ work = false; }
};