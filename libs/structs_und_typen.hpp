struct server_configure {
    int port;
    int maxClinents;    
};
struct req10_t { // option form
    uint8_t code0, code1, code2, code3, code4, code5, code6, code7, code8, code9;
    uint8_t full[10];

    req10_t(const uint8_t options[10]) {
         for (int i = 0; i < 10; ++i) {
            full[i] = options[i];
        }
        code0=full[0]; code1=full[1]; code2=full[2];
        code3=full[3]; code4=full[4]; code5=full[5];
        code6=full[6]; code7=full[7]; code8=full[8];
        code9=full[9];
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
    char sey_main[10];
    
    sey_t(char   sey[10]){
        memcpy(sey_main, sey, 10);
      
    }
    ~sey_t(){
     
    }

    bool operator==(const char* other) const {
        return strncmp(sey_main, other, 10) == 0;
    }
    
};

struct client_connection_data {
    const time_t time_start;         
    time_t last_activity_time; 
    req10_t client_options;
    sey_t sey;
    
    client_connection_data(time_t t, req10_t opts, sey_t s) 
        : time_start(t), last_activity_time(t), client_options(opts), sey(s) {};
};


struct hex_t{ // hex of userData
    char* hexe;
    hex_t (client_connection_data d){
        std::stringstream ss;
        ss << std::hex << d.time_start;
        for (int i = 0; i < 10 && d.sey.sey_main[i] != '\0'; ++i) {
            ss << std::setw(2) << std::setfill('0') 
               << std::hex << (int)(unsigned char)d.sey.sey_main[i];
        }
        std::string hexStr = ss.str();
        hexe = strdup(hexStr.c_str());
    }
    ~hex_t() {
        free(hexe);
    }
    bool operator<(const hex_t& other) const {
        return strcmp(hexe, other.hexe) < 0;
    }
};

struct client_packet_raw{
    uint8_t client_options[10];// 0-10
    char    client_sey[20];    // 10-20;
    uint8_t data_size[2];      // 20-22
    // data 22+

};

