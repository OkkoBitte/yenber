namespace SeyController{
    class seyManager{   
        private:
        std::map<hex_t, sey_t> seys;
        std::string pathToSey;

        void seyDumps(){
            
        }

        public:
        seyManager(server_configure sc) {
            pathToSey = sc.pathClientsData;
            seyDumps();
        }

        std::optional<sey_t> getSey(sey_t seyf){
            
            return std::nullopt;
        }
        
        void addSey(hex_t clientHx, sey_t sey){

        }


    };

    bool isSey (SeyController::seyManager& sm, sey_t sey){
        return (sm.getSey(sey)!= std::nullopt)? true : false; // ?(a ?= b)
    };       
};

namespace serverConfigureController{
    server_configure getConfigFropPath(std::string path){
        if(!file::is_file(path)){
            std::string errort = "not find file '" + path+"'";
            log::err(errort);
            std::exit(STATUS_OPERATION_ERROR);
        }
        
        std::string file_data = file::read_file<std::string>(path);
        std::vector<vars> kvar = kconf(file_data, {});
        server_configure retconf;

      
        retconf.maxClients = 15;
        retconf.sleepClients = 10;
        retconf.maxSleepClients = 20;
        retconf.maxGetClientPacket = 1000000;
        retconf.maxRetySendingPacket = 10;

        bool hasPort = false;
        bool hasPathClientsData = false;

        for (auto var : kvar){
            if(var.name == "port"){
                if (var.valib.type == VSID::intV){
                    retconf.port = atoi(var.valib.value.c_str());
                    hasPort = true;
                }
                else{
                    log::err("var 'port' need type INT");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            else if (var.name == "maxClients"){
                if (var.valib.type == VSID::intV){
                    retconf.maxClients = atoi(var.valib.value.c_str());
                }
                else{
                    log::err("var 'maxClients' need type INT");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            else if (var.name == "sleepClients"){
                if (var.valib.type == VSID::intV){
                    retconf.sleepClients = atoi(var.valib.value.c_str());
                }
                else{
                    log::err("var 'sleepClients' need type INT");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            else if (var.name == "maxSleepClients"){
                if (var.valib.type == VSID::intV){
                    retconf.maxSleepClients = atoi(var.valib.value.c_str());
                }
                else{
                    log::err("var 'maxSleepClients' need type INT");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            else if (var.name == "maxGetClientPacket"){
                if (var.valib.type == VSID::intV){
                    retconf.maxGetClientPacket = atoi(var.valib.value.c_str());
                }
                else{
                    log::err("var 'maxGetClientPacket' need type INT");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            else if (var.name == "maxRetySendingPacket"){
                if (var.valib.type == VSID::intV){
                    retconf.maxRetySendingPacket = atoi(var.valib.value.c_str());
                }
                else{
                    log::err("var 'maxRetySendingPacket' need type INT");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            else if (var.name == "pathClientsData"){
                if (var.valib.type == VSID::stringV){
                    retconf.pathClientsData = var.valib.value;
                    hasPathClientsData = true;
                }
                else{
                    log::err("var 'pathClientsData' need type STRING");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
        }
    
        if(!hasPort) {
            log::err("Configuration error: 'port' is required but not specified");
            std::exit(STATUS_OPERATION_ERROR);
        }
        
        if(!hasPathClientsData) {
            log::err("Configuration error: 'pathClientsData' is required but not specified");
            std::exit(STATUS_OPERATION_ERROR);
        }
        
        return retconf;
    }
};



uint8_t generate_random_byte() {

    static std::random_device rd;  
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255); 
    
    return static_cast<uint8_t>(dis(gen)); 
}

namespace PacketController{
    class packetManager{
        
        std::vector<packContoll> isMy; 
        
        std::vector<packetActions> actions;

        void isOk() {
            const size_t maxElements = 1000; 
            const size_t maxBytes =  10 * 1024 * 1024; 

            size_t totalBytes = isMy.size() * sizeof(packContoll) + 
                                actions.size() * sizeof(packetActions);

            if (isMy.size() > maxElements || actions.size() > maxElements || totalBytes > maxBytes) {
                isMy.clear();
                actions.clear();
                actions.push_back(packetActions({action_e::close_client, {}}));
                log::err("Memory limit exceeded"); 
            }
        }
        public:
            std::optional<server_configure> sconf;
            void postHe(packet_s &packet, std::vector<uint8_t> &data){
                isOk();
                if (*packet.type == packet_type::menegmend) { // [ACK]
                    isMy.erase(
                        std::remove_if(
                            isMy.begin(),
                            isMy.end(),
                            [&packet](const packContoll& pc) {
                                return memcmp(pc.packet_head.hxcode, packet.hxcode, sizeof(packet.hxcode)) == 0;
                            }
                        ),
                        isMy.end()
                    );
                }
                else if(*packet.type == packet_type::control){ 
                    if (!data.empty()){
                        if (data.front() == packet_controll::close){
                            actions.push_back(packetActions({action_e::close_client,{}}));
                        }  
                    }
                }
                else if(*packet.type == packet_type::data){
                    packet_s packetHead;
                    packContoll pcoll;
                    packetActions pactoin;
         
                    packetHead.type     [0] = packet_type::menegmend;
                    packetHead.hxcode   [0] = packet.hxcode[0];
                    packetHead.hxcode   [1] = packet.hxcode[1];
                    packetHead.datasize [0] = 0x00;
                    packetHead.datasize [1] = 0x00;
                    
                    pcoll.time = static_cast<int>(std::time(nullptr));
                    pcoll.packet_head=packetHead; 
                    pcoll.data = {};
                    
                    pactoin.action = action_e::send_data;
                    pactoin.packet = pcoll;

                    actions.push_back(pactoin);

                    packetActions newActoin;
                    packContoll newPcoll;
                    newPcoll.packet_head = packet;
                    newPcoll.data = data;
                    newActoin.action = action_e::get_data;
                    newActoin.packet = newPcoll;
                    actions.push_back(newActoin);
                } 

            };
            void postMy(packet_s &packet, std::vector<uint8_t> &data){
                isOk();
                packetActions pactoin;
                pactoin.action = action_e::send_data;

                packContoll newPcoll;
                newPcoll.time = time(nullptr);
                newPcoll.packet_head = packet;
                newPcoll.data = data;

                pactoin.packet = newPcoll;
                isMy.push_back(newPcoll);
                actions.push_back(pactoin);
            };

            void sendInfoClose(uint8_t why){
                packet_s packetHead;
                packContoll pcoll;
                packetActions pactoin;
        
                packetHead.type     [0] = packet_type::control;
                packetHead.hxcode   [0] = 0x00;
                packetHead.hxcode   [1] = 0x00;
                packetHead.datasize [0] = 0x01;
                packetHead.datasize [1] = 0x00;
                
                pcoll.time = static_cast<int>(std::time(nullptr));
                pcoll.packet_head=packetHead; 
                pcoll.data = {why};
                
                pactoin.action = action_e::close_client;
                pactoin.packet = pcoll;

                actions.push_back(pactoin);


            };

            std::vector<packetActions> managment_packets() {
        
                std::vector<packetActions> acp;
                acp.swap(actions); 
                
                time_t current_time = time(nullptr);
                auto it = isMy.begin();
                
                while(it != isMy.end()) {
                    
                    uint8_t timeout_seconds = it->packet_head.timeout[0];
                    
                    if((current_time - it->time) > timeout_seconds) {
                        if(it->retry_count < (sconf?sconf->maxRetySendingPacket:5)) {
                            
                            it->time = current_time;
                            it->retry_count++;
                            
                            
                            packetActions pact;
                            pact.action = action_e::send_data;
                            pact.packet = *it;
                            acp.push_back(pact);
                            
                            ++it;
                        } else {
                            it = isMy.erase(it);
                        }
                    } else {
                        ++it;
                    }
                }
                
                return acp;
            }
    };
}