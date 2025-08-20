#define NULL_STR ""

#define VERSION_CODE 40
#define VERSION_NAME_SERVER "0.1.1-release[stable]"
#define VERSION_NAME_CLIENT "0.1.1-release[stable]"

#define DESCRIBE_VERSION_YENBER "0.1.0-debuge[inDev]"

#define STATUS_SYSTEM_OPERATION_ERROR -1
#define STATUS_OPERATION_OK 0
#define STATUS_OPERATION_ERROR 1

static constexpr const char* AP__KRR__CONF       = "1";
static constexpr const char* BIN__KRR__CONF      = "0";
static constexpr const char* FO__KRR__CONF       = "-1";


uint8_t mazor_code = 0x01;


const std::string help1_server = "|[Neum:V][HELP] server config.krr   \n\n\tfor get more data --describe\n\tfor get help --help\n";
std::string help2_server = "|[Neum:V][DESCRIBE] \n\n\tVersion Code "+std::to_string(VERSION_CODE)+"\n\tVersion Name "+VERSION_NAME_SERVER+"\n\t Yenber "+DESCRIBE_VERSION_YENBER+"\n\n More web-mbg.ru\n\n";

template <typename G>
void hxout_struct(G gdata){
        std::cout<<"THERE: -----------/---------STURCT----"<<std::endl;
        std::vector<uint8_t> data(reinterpret_cast<uint8_t*>(&gdata), reinterpret_cast<uint8_t*>(&gdata) + sizeof(gdata));
        for (auto d : data) {
            std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(d) << " ";
        }
        std::cout << std::dec << std::endl; 
        for (auto d : data) {
            std::cout  << static_cast<char>(d) << " ";
        }
        std::cout << std::dec << std::endl;
}
void hxout_data(std::vector<uint8_t> data){
      std::cout<<"THERE: -----------/--------DATA--"<<std::endl;
    for (auto d : data) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(d) << " ";
    }
    std::cout << std::dec << std::endl; 
    for (auto d : data) {
        std::cout  << static_cast<char>(d) << " ";
    }
    std::cout << std::dec << std::endl;
}