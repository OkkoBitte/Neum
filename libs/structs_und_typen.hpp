struct server_configure {
    int port;             // port server
    int maxClients;       // max Managment clinet
    int sleepClients;     // what seconds client passiving
    int maxSleepClients;  // max seconds client passiving

    int maxGetClientPacket; // max packet size of client data

    std::string pathClientsData; // path where saving/uploading clinet data
};
struct req10_t { // option form
    uint8_t code1, code2, code3, code4, code5, code6, code7, code8, code9, code10;
    uint8_t full[10];

    req10_t(const uint8_t options[10]) {
        for (int i = 0; i < 10; ++i) {
            full[i] = options[i];
        }
        code1=full[0]; // what client (new/old) // 0x01 - обсолютно новый, 0x02 - повторное (может из за ошибки)
        code2=full[1]; // what type connection (get-close/constant)
        code3=full[2]; // what action of passivation client (wait/closing)
        code4=full[3]; // type connection (passive/activ)
        code5=full[4]; 
        code6=full[5];
        code7=full[6];
        code8=full[7]; 
        code9=full[8];  
        code10=full[9]; 
    }


    bool operator==(const uint8_t other[10]) const {
        return memcmp(full, other, sizeof(full)) == 0;
    }
    friend std::ostream& operator<<(std::ostream& os, const req10_t& req) {
        os << std::hex << std::setfill('0');  
        for (int i = 0; i < 10; ++i) {
            os << std::setw(2) << static_cast<int>(req.full[i]);
            if (i < 9) os << " "; 
        }
        os << std::dec;  
        return os;
    }

};
  
struct sey_t{ // session key
    char sey_main[20];
    
    sey_t(char   sey[20]){ memcpy(sey_main, sey, 20); }

    bool operator==(sey_t &other) const { return strncmp(sey_main, other.sey_main, 20) == 0; }
    
};

struct client_connection_data {
    time_t time_start;         
    time_t last_activity_time; 
    req10_t client_options;
    sey_t sey;
    int desc;

    sockaddr_in clinet_sock;

    client_connection_data(time_t t, req10_t opts, sey_t s, int d, sockaddr_in sci) 
        : time_start(t), last_activity_time(t), client_options(opts), sey(s),desc(d), clinet_sock(sci){};//  
};


struct hex_t {
    std::string hexe; 

    hex_t(const client_connection_data& d) {
        std::stringstream ss;
        ss << std::hex << d.time_start;
        for (int i = 0; i < 10 && d.sey.sey_main[i] != '\0'; ++i) {
            ss << std::setw(2) << std::setfill('0') 
               << std::hex << (int)(unsigned char)d.sey.sey_main[i];
        }
        hexe = ss.str();  
    }

   
    bool operator<(const hex_t& other) const { 
        return hexe < other.hexe; 
    }
};

struct client_head_packet_raw{
    uint8_t client_options[10];// 0-10
    char    client_sey[20];    // 10-20;

};

struct packet_s{
    uint8_t type[1];       // 0
    uint8_t hxcode[2];     // 1-2
    uint8_t timeout[1];    // 3
    uint8_t datasize[2];   // 4-5
                           // 6++

};
