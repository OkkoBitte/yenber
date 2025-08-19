class clientManager;
class hostManager;

class clientManager {
public:
    virtual ~clientManager() = default;
    virtual void getData(std::vector<uint8_t>) = 0;
    
    virtual void connected() = 0 ;
    virtual void closeConnection()=0;
    void sendData(std::vector<uint8_t> data) {
        size_t data_size = data.size();
        packet_s phead;
        phead.type[0] = packet_type::data;
        phead.hxcode[0] = rand() % 256;
        phead.hxcode[1] = rand() % 256;
        phead.timeout[0] = 0x0A;
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

        if (send(sockfd, &chpr, sizeof(chpr), MSG_WAITALL) != sizeof(chpr))  { 
            log::warn("Failed to send client header");
        }

        
        startCommunication();
        return true;
    }

    void startCommunication() {
        std::thread cdl([&](){ manager->connected(); });
        cdl.detach();

        
        int flag = 1;
        setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        while (workojob) {
            std::vector<packetActions> pactos;
            {
                std::lock_guard<std::mutex> lock(manager->pmut);
                pactos = manager->pmanager.managment_packets();
            }
            for (auto& act : pactos) {
                switch (act.action) {
                    case action_e::close_client:
                        workojob = false;
                        manager->closeConnection();
                        break;
                        
                    case action_e::get_data:
                  
                        manager->getData(act.packet.data);
                 
                        break;
                        
                    case action_e::send_data: {
                       
                        std::vector<uint8_t> full_packet;
                        full_packet.reserve(sizeof(packet_s) + act.packet.data.size());
                        full_packet.insert(full_packet.end(), 
                                        (uint8_t*)&act.packet.packet_head, 
                                        (uint8_t*)&act.packet.packet_head + sizeof(packet_s));
                        full_packet.insert(full_packet.end(), 
                                        act.packet.data.begin(), 
                                        act.packet.data.end());
                        send(sockfd, full_packet.data(), full_packet.size(), 0);
          
                
                        break;
                    }
                }
            }

            
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(sockfd, &read_fds);
            
            struct timeval select_tv = {0, 10000};
            
            if (select(sockfd + 1, &read_fds, NULL, NULL, &select_tv) > 0) {
                if (FD_ISSET(sockfd, &read_fds)) {
                    packet_s packet;
                    ssize_t received = recv(sockfd, &packet, sizeof(packet), MSG_WAITALL);
           

                    if (received == sizeof(packet)) {
                        
                        
                        uint16_t size = (packet.datasize[1] << 8) | packet.datasize[0];
                        if (size > 0) {
                            std::vector<uint8_t> data(size);
                            if (recv(sockfd, data.data(), size, MSG_WAITALL) == size) {
                        
               
                                manager->pmanager.postHe(packet, data);
                           
                            }
                        }
                        else{
                            std::vector<uint8_t> data;
                            manager->pmanager.postHe(packet, data);
                        }
                    } else if (received == 0) {
                        log::def("Server closed connection");
                        workojob = false;
                    }
                }
            }
        }

        log::def("Connection closed");
    }

};



void clientManager::loop() { 
    while (true) { 
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
