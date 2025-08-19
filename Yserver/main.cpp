#include <includelib.hpp>
#include <center.hpp>

std::string confpathp;
class MyServerManager : public serverManager {
private: 
    mutable std::mutex mutex;
    std::map<sey_t, hex_t> cs;

    void addCs(sey_t sey, hex_t hex) {
        std::lock_guard<std::mutex> lock(mutex);
        cs[sey] = hex; 
    }
    
    std::optional<hex_t> getCs(sey_t sey) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = cs.find(sey);
        if(it != cs.end()) return it->second;
        return std::nullopt;
    }

public:
    MyServerManager() : serverManager(serverConfigureController::getConfigFropPath(confpathp)) {}
    
    void newClientConnection(hex_t client_hex) override {    
        auto client = getClient(client_hex);
        if (client) {
            addCs(client->sey, client_hex);        
            std::cout << "CONNECTED: " 
                      << std::string(client->sey.sey_main, 20) 
                      << " [" << client->desc << "]" << std::endl;
        }
    }

    void getClientData(hex_t sender_hex, const std::vector<uint8_t>& data) override {
        if (data.size() < 21) return;

        sey_t receiver_sey;
        memcpy(receiver_sey.sey_main, data.data(), 20);

        auto receiver_hex_opt = getCs(receiver_sey);
        if (!receiver_hex_opt) return;

        auto sender = getClient(sender_hex);
        if (!sender) return;

        std::vector<uint8_t> modified_data;
        modified_data.reserve(20 + data.size() - 20);
        
       
        modified_data.insert(modified_data.end(),  sender->sey.sey_main, sender->sey.sey_main + 20);
   
        modified_data.insert(modified_data.end(), data.begin() + 20, data.end());

        
        
        sendData(*receiver_hex_opt, modified_data);
    }
};

int main(int argc, char* argv[]) {  
    
    if (argc == 1){
        std::cout << "|[Yenber] "<<argv[0]<<" <filename> or --help"<<std::endl;
    }
    else if(argc > 1){
        std::string bedl = std::string (argv[1]);

        if(bedl == "--help"){
            std::cout<<help1_server;
        }
        else if(bedl == "--describe"){
            std::cout<<help2_server;
        }
        else{
            confpathp = bedl;
            MyServerManager myapp;
    

            myapp.loop();
           
            
        }

    }
}

