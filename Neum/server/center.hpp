
class serverManager;
class hostManagerInterface {
public:
    virtual ~hostManagerInterface() = default;
    virtual void putClienPcm(hex_t hexCode, PacketController::packetManager& pcol) = 0;
    virtual PacketController::packetManager* getClienPcm(hex_t hexCode) = 0;
    virtual  void deleteClientPcm(hex_t hexCode) =0;
    virtual bool startSocketServer() = 0;
    virtual bool stopSocketServer() = 0;
    virtual void handleClient(hex_t client_hex) = 0;
    std::mutex map_mutex;
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


public:
    hostManager(serverManager* mgr);
    ~hostManager() override;
    std::mutex map_mutex;
    void putClienPcm(hex_t hexCode, PacketController::packetManager& pcol) override;
    PacketController::packetManager* getClienPcm(hex_t hexCode) override;
    void deleteClientPcm(hex_t hexCode) override;
    bool startSocketServer() override;
    bool stopSocketServer() override;
    void handleClient(hex_t client_hex) override;
    bool reStartSocketServer();

private:
    void processIncomingData(hex_t client_hex, const std::vector<uint8_t>& data);
};


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

        PacketController::packetManager *pm = hmanager->getClienPcm(hexClient);

        if(pm){
            pm->sendInfoClose(close_connection_info::youerror);
        }


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
    PacketController::packetManager *pm = hmanager->getClienPcm(hexClient);

    if(pm){
        pm->sendInfoClose(close_connection_info::youerror);
    }
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

    constexpr int SLEEP_MS = 1000;
    
    while (work) {
        auto start_time = std::chrono::steady_clock::now();
        
      
        std::vector<std::tuple<hex_t, req10_t, time_t, sey_t, int>> clients_data;
        {
            std::lock_guard<std::mutex> lock(map_mutex);
            clients_data.reserve(virtual_connections.size());
            for (const auto& [hex, data] : virtual_connections) {
                clients_data.emplace_back(hex, data.client_options, 
                                        data.last_activity_time, 
                                        data.sey, 
                                        data.desc);
            }
        }

       
        std::vector<hex_t> to_purge;
        const time_t now = time(nullptr);
        const int sleep_threshold = serverConfigure.sleepClients;
        const int max_sleep_threshold = serverConfigure.maxSleepClients;

        for (const auto& [hex, co, last_activity, sey, desc] : clients_data) {
            const time_t inactive_time = now - last_activity;
            
            if (inactive_time > sleep_threshold && (co.code3 == 0x00 || (inactive_time > static_cast<int>(co.code3) &&  static_cast<int>(co.code3) > max_sleep_threshold))) {
                to_purge.push_back(hex);
                log::warn("PURGE TIMEOUT: " + std::string(sey.sey_main,20) + " [" + std::to_string(desc) + "]");
            }
        }

        
        if (!to_purge.empty()) {
            std::lock_guard<std::mutex> lock(map_mutex);
            for (const auto& hex : to_purge) {
                purgeClient_unsafe(hex);
            }
        }

        
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto sleep_duration = std::chrono::milliseconds(SLEEP_MS) -  std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
        
        if (sleep_duration.count() > 0) {
            std::this_thread::sleep_for(sleep_duration);
        }
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
void  hostManager::deleteClientPcm(hex_t hexCode){
    std::lock_guard<std::mutex> lock(map_mutex);
    auto it = packe_manager.find(hexCode);
    if(it != packe_manager.end())  packe_manager.erase(it) ;
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

        if (header.client_options[9] != mazor_code){
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
        PacketController::packetManager pmanager;
        pmanager.sconf = manager->serverConfigure;
        putClienPcm(client_hex, pmanager);
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
        int maxPacketSize = manager->serverConfigure.maxGetClientPacket;
        PacketController::packetManager* pmanager = getClienPcm(client_hex);
        if (!pmanager) {
            log::err("Client packet manager not found");
            return;
        }

    
        int flag = 1;
        setsockopt(ccd->desc, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

        while (isActive) {
            std::vector<packetActions> pactos ;
            {
                std::lock_guard<std::mutex> lock(map_mutex);
                pactos = pmanager->managment_packets();
            }
            for (auto& act : pactos) {
                if (act.action == action_e::close_client) {
                    log::def("Closing client "+std::to_string(ccd->desc));
                    deleteClientPcm(client_hex);
                    return;
                }
                else if (act.action == action_e::get_data) {
                    manager->getClientData(client_hex, act.packet.data);
                }
                else if (act.action == action_e::send_data) {
    
                    std::vector<uint8_t> full_packet;
                    full_packet.insert(full_packet.end(), 
                                     (uint8_t*)&act.packet.packet_head, 
                                     (uint8_t*)&act.packet.packet_head + sizeof(packet_s));
                    full_packet.insert(full_packet.end(), 
                                     act.packet.data.begin(), 
                                     act.packet.data.end());
                    send(ccd->desc, full_packet.data(), full_packet.size(), 0);



                }
            }

            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(ccd->desc, &read_fds);
            struct timeval tv = {0, 10000};

            if (select(ccd->desc + 1, &read_fds, NULL, NULL, &tv) <= 0) {
                continue;
            }

            packet_s packet;
            ssize_t received = recv(ccd->desc, &packet, sizeof(packet), MSG_WAITALL);
            
            if (received <= 0) {
                if (received == 0) {
                    log::def("Client disconnected: "+std::to_string(ccd->desc));
                } else {
                    log::warn("Receive error: "+std::to_string(errno)+", Free.");
                }
                break;
            }


            ccd->last_activity_time = time(nullptr);
            manager->mdfClient(client_hex, *ccd);
        
          
            uint16_t size = (packet.datasize[1] << 8) | packet.datasize[0];
            if (size > 0) {
                if (size > maxPacketSize) {
                    log::err("Packet too large: "+std::to_string(size));
                    break;
                }

                std::vector<uint8_t> data(size);
                received = recv(ccd->desc, data.data(), size, MSG_WAITALL);
                if (received != size) {
                    log::warn("Incomplete data received");
                    continue;
                }

                std::lock_guard<std::mutex> lock(map_mutex);
                pmanager->postHe(packet, data);
            } else {
                std::vector<uint8_t> data;
                std::lock_guard<std::mutex> lock(map_mutex);
                pmanager->postHe(packet, data);
            }
        }
    } catch (const std::exception& e) {
        log::err("Client handler crashed: " + std::string(e.what()));
    }
    
    deleteClientPcm(client_hex);
};
void serverManager::sendData(hex_t clientHx, std::vector<uint8_t> data){
    PacketController::packetManager* pm =  hmanager->getClienPcm(clientHx);
  
    if(pm){
        size_t data_size = data.size();
        packet_s phead;
        phead.type[0] = packet_type::data;
        phead.hxcode[0] = generate_random_byte();
        phead.hxcode[1] = generate_random_byte();
        phead.timeout[0] = 0x0A;
        phead.datasize[0] = static_cast<uint8_t>(data_size & 0xFF);        
        phead.datasize[1] = static_cast<uint8_t>((data_size >> 8) & 0xFF); 
      
        std::lock_guard<std::mutex> (hmanager->map_mutex);
        pm->postMy(phead,data);

    }
    else{
        log::err ("Dont get client from send data");
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
