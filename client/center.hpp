class clientManager;
class hostManager;

class clientManager {
public:
    virtual ~clientManager() = default;
    virtual void getData(std::vector<uint8_t>) = 0;
    
    virtual void connected() = 0 ;

    void sendData(std::vector<uint8_t> data) {
        size_t data_size = data.size();
        packet_s phead;
        phead.type[0] = packet_type::data;
        phead.hxcode[0] = rand() % 256;
        phead.hxcode[1] = rand() % 256;
        phead.datasize[0] = static_cast<uint8_t>(data_size & 0xFF);
        phead.datasize[1] = static_cast<uint8_t>((data_size >> 8) & 0xFF);
        std::lock_guard<std::mutex> lock(pmut);
        pmanager.postMy(phead, data);
    }
    void loop();
    client_configure& config;
    std::mutex pmut;
    PacketController::packetManager pmanager;

protected:
    clientManager(client_configure& conf) : config(conf) {}
    
};

class hostManager {
public:
    hostManager(clientManager* manager) : manager(manager) {
        std::thread cnn([&](){

            if (!connect()) throw std::runtime_error("Connection failed");
            
        });
        cnn.detach();
        
    }

    ~hostManager() {
        workojob = false;
        if (sockfd != -1) {
            close(sockfd);
        }
    }

private:
    clientManager* manager;
    std::atomic<bool> workojob{false};
    int sockfd = -1;

    bool connect() {
        if (!manager) {
            log::err("Manager is not initialized");
            return false;
        }

        const std::string& hostname = manager->config.hostname;
        const int port = manager->config.port;

        if (hostname.empty()) {
            log::err("Hostname is empty");
            return false;
        }

        if (port <= 0 || port > 65535) {
            log::err("Invalid port: " + std::to_string(port));
            return false;
        }

        struct addrinfo hints = {};
        struct addrinfo* result = nullptr;
        
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        int status = getaddrinfo(hostname.c_str(), std::to_string(port).c_str(), &hints, &result);
        if (status != 0) {
            log::err("DNS lookup failed: " + std::string(gai_strerror(status)));
            return false;
        }

        std::unique_ptr<struct addrinfo, decltype(&freeaddrinfo)> addr_info(result, &freeaddrinfo);

        for (struct addrinfo* p = result; p != nullptr; p = p->ai_next) {
            sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sockfd < 0) continue;

            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;
            
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

            log::def("Connecting to " + hostname + ":" + std::to_string(port));
            
            if (::connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)  break;
            

            close(sockfd);
            sockfd = -1;
        }

        if (sockfd < 0) {
            log::err("Connection failed: " + std::string(strerror(errno)));
            return false;
        }
        
        workojob = true;
        client_head_packet_raw chpr;
        memcpy(chpr.client_options, manager->config.options.full, sizeof(chpr.client_options));
        memcpy(chpr.client_sey, manager->config.sey.sey_main, sizeof(chpr.client_sey));

        if (send(sockfd, &chpr, sizeof(chpr), 0) != sizeof(chpr))         log::warn("Failed to send header");
    
        
        startCommunication();
        return true;
    }

    void startCommunication() {
        std::thread cll([&](){manager->connected();});
        cll.detach();

        while (workojob) {
            std::vector<packetActions> pactos = manager->pmanager.managment_packets();

            for (auto& act : pactos) {
                if (act.action == action_e::close_client) {
                    workojob = false;
                    break;
                }
                else if (act.action == action_e::get_data) {
                    manager->getData(act.packet.data);
                }
                else if (act.action == action_e::send_data) {
                    packet_s head = act.packet.packet_head;
                    std::vector<uint8_t> data = act.packet.data;
                    
                    if (send(sockfd, &head, sizeof(head), 0) != sizeof(head)) {
                        log::warn("Failed to send packet header");
                        continue;
                    }
                    
                    if (!data.empty() && send(sockfd, data.data(), data.size(), 0) != data.size()) {
                        log::warn("Failed to send packet data");
                        continue;
                    }
                }
            }

            packet_s packet;
            if (recv(sockfd, &packet, sizeof(packet), MSG_WAITALL) != sizeof(packet)) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                continue;
            }

            uint16_t size = (packet.datasize[1] << 8) | packet.datasize[0];
            std::vector<uint8_t> data(size);
            
            if (size > 0 && recv(sockfd, data.data(), size, MSG_WAITALL) != size) {
                log::warn("Failed to read packet data");
                continue;
            }
                
            manager->pmanager.postHe(packet, data);
        }

        log::def("Connection closed");
    }
};

void clientManager::loop() { 
    while (true) { 
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
