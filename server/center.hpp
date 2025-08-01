
class serverManager;
namespace PacketController { class packetManager; }
struct hex_t;
struct client_connection_data;
struct server_configure;

class hostManagerInterface {
public:
    virtual ~hostManagerInterface() = default;
    virtual void putClienPcm(hex_t hexCode, PacketController::packetManager& pcol) = 0;
    virtual PacketController::packetManager* getClienPcm(hex_t hexCode) = 0;
    virtual bool startSocketServer() = 0;
    virtual bool stopSocketServer() = 0;
    virtual void handleClient(hex_t client_hex) = 0;
};

class serverManager {

public:
    std::map<hex_t, client_connection_data> virtual_connections;
    std::mutex map_mutex;
    std::atomic<bool> work{true};
    std::unique_ptr<hostManagerInterface> hmanager;
    SeyController::seyManager smanager;
    server_configure serverConfigure;

    client_connection_data* getClient_unsafe(hex_t hexClient);
    bool mdfClient_unsafe(hex_t hexClient, const client_connection_data& newData);
    bool purgeClient_unsafe(hex_t hexClient);
    bool dltClient_unsafe(hex_t hexClient);

    serverManager(server_configure config);
    virtual ~serverManager() = default;
    virtual void newClientConnection(hex_t client_hex) = 0;
    virtual void getClientData(hex_t client_hex, const std::vector<uint8_t>& data) = 0;

    void sendData(hex_t clientHx, std::vector<uint8_t> data);
    bool addClient(const client_connection_data& newData);
    client_connection_data* getClient(hex_t hexClient);
    bool mdfClient(hex_t hexClient, const client_connection_data& newData);
    bool dltClient(hex_t hexClient);
    bool purgeClient(hex_t hexClient);
    size_t getClientConnectionsCount();
    void MENEGMANT_CLIENTS();
    void stop();
    void loop();
};

class hostManager : public hostManagerInterface {
private:
    serverManager* manager;
    int socket_desc;
    std::atomic<bool> isActive{false};
    std::map<hex_t, PacketController::packetManager> packe_manager;
    std::mutex map_mutex;

public:
    hostManager(serverManager* mgr);
    ~hostManager() override;

    void putClienPcm(hex_t hexCode, PacketController::packetManager& pcol) override;
    PacketController::packetManager* getClienPcm(hex_t hexCode) override;
    bool startSocketServer() override;
    bool stopSocketServer() override;
    void handleClient(hex_t client_hex) override;
    bool reStartSocketServer();

private:
    void processIncomingData(hex_t client_hex, const std::vector<uint8_t>& data);
};