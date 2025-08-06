#define NULL_STR ""

#define VERSION_CODE 7
#define VERSION_NAME_SERVER "0.0.5-start"
#define VERSION_NAME_CLIENT "0.0.2-start"



#define STATUS_SYSTEM_OPERATION_ERROR -1
#define STATUS_OPERATION_OK 0
#define STATUS_OPERATION_ERROR 1

static constexpr const char* AP__KRR__CONF       = "1";
static constexpr const char* BIN__KRR__CONF      = "0";
static constexpr const char* FO__KRR__CONF       = "-1";

const std::string help1_server = "|[Neum:V][HELP] server config.krr   \n\n\tfor get more data --describe\n\tfor get help --help\n";
std::string help2_server = "|[Neum:V][DESCRIBE] \n\n\tVersion Code "+std::to_string(VERSION_CODE)+"\n\tVersion Name"+VERSION_NAME_SERVER+"\n\n More web-mbg.ru";
