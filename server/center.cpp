#include "center.hpp"

serverManager::serverManager(server_configure config) 
    : serverConfigure(config), smanager(config), work(true) {
    hmanager = std::make_unique<hostManager>(this);
    log::def("Management start");
    std::thread managerThread([this]() { MENEGMANT_CLIENTS(); });
    managerThread.detach();
}

client_connection_data* serverManager::getClient_unsafe(hex_t hexClient) {
    auto it = virtual_connections.find(hexClient);
    return it != virtual_connections.end() ? &it->second : nullptr;
}

bool serverManager::mdfClient_unsafe(hex_t hexClient, const client_connection_data& newData) {
    auto it = virtual_connections.find(hexClient);
    if (it == virtual_connections.end()) {
        return false;
    }
    it->second = newData;
    return true;
}

bool serverManager::purgeClient_unsafe(hex_t hexClient) {
    try {
        auto it = virtual_connections.find(hexClient);
        if (it == virtual_connections.end()) {
            log::warn("Client not found");
            return false;
        }

        if (it->second.desc != -1) {
            const int fd = it->second.desc;
            it->second.desc = -1; 
            close(fd);
        }

        virtual_connections.erase(it);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error in purge: " << e.what() << std::endl;
        return false;
    }
}

bool serverManager::dltClient_unsafe(hex_t hexClient) {
    auto it = virtual_connections.find(hexClient);
    if (it == virtual_connections.end()) return false;
    virtual_connections.erase(it);
    return true;
}

bool serverManager::addClient(const client_connection_data& newData) {
    std::lock_guard<std::mutex> lock(map_mutex);
    auto hex = hex_t(newData);
    return virtual_connections.insert_or_assign(hex, newData).second;
}

client_connection_data* serverManager::getClient(hex_t hexClient) {
    std::lock_guard<std::mutex> lock(map_mutex);
    return getClient_unsafe(hexClient);
}

bool serverManager::mdfClient(hex_t hexClient, const client_connection_data& newData) {
    std::lock_guard<std::mutex> lock(map_mutex);
    return mdfClient_unsafe(hexClient, newData);
}

bool serverManager::dltClient(hex_t hexClient) {
    std::lock_guard<std::mutex> lock(map_mutex);
    return dltClient_unsafe(hexClient);
}

bool serverManager::purgeClient(hex_t hexClient) {
    std::lock_guard<std::mutex> lock(map_mutex);
    return purgeClient_unsafe(hexClient);
}

size_t serverManager::getClientConnectionsCount() {
    std::lock_guard<std::mutex> lock(map_mutex);
    return virtual_connections.size();
}

void serverManager::MENEGMANT_CLIENTS() {
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
                        std::string log_data = "PURGE TIMEOUT: " + std::string(data.sey.sey_main) + 
                                             " [" + std::to_string(data.desc) + "]";
                        log::warn(log_data);
                    }
                }
            }
        
            for (const auto& hex : to_remove) {
                purgeClient_unsafe(hex);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        when = (when > 9999) ? 0 : when + 1;
    }
}

void serverManager::stop() { 
    work = false;
    log::warn("stopping server Manager");
    if (hmanager) hmanager->stopSocketServer();
    else log::err("not initialized H-manager");
}

void serverManager::loop() { 
    while (work) { 
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


hostManager::hostManager(serverManager* mgr) 
    : manager(mgr), socket_desc(-1), isActive(false) {
    std::thread socketServerThread([this]() { startSocketServer(); });
    socketServerThread.detach();
}

hostManager::~hostManager() {
    stopSocketServer();
}

void hostManager::putClienPcm(hex_t hexCode, PacketController::packetManager& pcol) {
    std::lock_guard<std::mutex> lock(map_mutex);
    packe_manager.insert_or_assign(hexCode, pcol);
}

PacketController::packetManager* hostManager::getClienPcm(hex_t hexCode) {
    std::lock_guard<std::mutex> lock(map_mutex);
    auto it = packe_manager.find(hexCode);
    return it != packe_manager.end() ? &it->second : nullptr;
}

bool hostManager::startSocketServer() {
    if (isActive) return false;
    
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == STATUS_SYSTEM_OPERATION_ERROR) {
        log::err("Fatal error, not opening socket");
        std::exit(STATUS_OPERATION_ERROR);
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(manager->serverConfigure.port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_desc, (sockaddr*)&serverAddr, sizeof(serverAddr)) == STATUS_SYSTEM_OPERATION_ERROR) {
        log::err("Fatal error, not binding socket");
        close(socket_desc);
        std::exit(STATUS_OPERATION_ERROR);
    }

    if (listen(socket_desc, manager->serverConfigure.maxClients) == STATUS_SYSTEM_OPERATION_ERROR) {
        log::err("Fatal error, not listening socket");
        close(socket_desc);
        std::exit(STATUS_OPERATION_ERROR);
    }

    log::def("server start, port " + std::to_string(manager->serverConfigure.port));
    isActive = true;

    while (isActive) {
        sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        int clientSocket = (manager->getClientConnectionsCount() < manager->serverConfigure.maxClients) 
            ? accept(socket_desc, (sockaddr*)&clientAddr, &clientAddrSize) 
            : STATUS_SYSTEM_OPERATION_ERROR;

        if (clientSocket == STATUS_SYSTEM_OPERATION_ERROR) {
            log::warn("Error accepting connection");
            continue;
        }

        time_t now = time(nullptr);
        client_head_packet_raw header;
        
        if (recv(clientSocket, &header, sizeof(header), MSG_WAITALL) != sizeof(header)) {
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
        
        hex_t client_hex = hex_t(new_client);
            
        if (!manager->addClient(new_client)) {
            log::warn("Failed to add client to manager");
            close(clientSocket);
            continue;
        }
        
        manager->newClientConnection(client_hex);
        std::thread([this, client_hex]() {
            this->handleClient(client_hex);
        }).detach();
    }
    
    log::def("stopped socket server");
    return true;
}

void hostManager::handleClient(hex_t client_hex) {
    try {
        auto ccd = manager->getClient(client_hex);   
        if (!ccd) return; 
        
        ccd->last_activity_time = time(nullptr);
        manager->addClient(*ccd);
        
        PacketController::packetManager pmanager;
        putClienPcm(client_hex, pmanager);

        while (isActive) {
            pmanager.managment_packets();
            packet_s packet;
            
            if (recv(ccd->desc, &packet, sizeof(packet), MSG_WAITALL) != sizeof(packet)) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                continue;
            }

            ccd->last_activity_time = time(nullptr);
            manager->addClient(*ccd);

            uint16_t size = (packet.datasize[1] << 8) | packet.datasize[0];
            std::vector<uint8_t> data(size);
            
            if (recv(ccd->desc, data.data(), size, MSG_WAITALL) != size) {
                log::warn("Failed to read packet data");
                continue;
            }
            
            pmanager.postHe(packet, data);
            manager->getClientData(client_hex, data);
        }
    } catch (const std::exception& e) {
        log::err(std::string("Client error: ") + e.what());
    }
}

bool hostManager::stopSocketServer() {
    std::string wtxt = "stopping host in port " + std::to_string(manager->serverConfigure.port);
    log::warn(wtxt);
    
    if (isActive) {
        isActive = false;
        return close(socket_desc) == 0;
    }
    return false;
}

bool hostManager::reStartSocketServer() {
    stopSocketServer();
    return startSocketServer();
}

void hostManager::processIncomingData(hex_t client_hex, const std::vector<uint8_t>& data) {

}