#define NULL_STR ""

#define VERSION_CODE 2
#define VERSION_NAME_SERVER "0.0.1-start"
#define VERSION_NAME_CLIENT "0.0.1-start"



#define STATUS_SYSTEM_OPERATION_ERROR -1
#define STATUS_OPERATION_OK 0
#define STATUS_OPERATION_ERROR 1

static constexpr const char* AP__KRR__CONF       = "1";
static constexpr const char* BIN__KRR__CONF      = "0";
static constexpr const char* FO__KRR__CONF       = "-1";

const std::string help1_server = "|[Neum:V][GAL] server config.krr   \n\n\tten lixt urlib --vocem\n\tten lixt gal --gal\n\n Ten lint kamen regwun, un inwer sutun lib, vane given apt;";
std::string help2_server = "|[Neum:V][VOCEM] \n\n\tVersion Code "+std::to_string(VERSION_CODE)+"\n\tVersion Name"+VERSION_NAME_SERVER+"\n\n More web-mbg.ru";