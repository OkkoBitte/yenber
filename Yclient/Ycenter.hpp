namespace yClientConfigController{

    client_configure getConfig(std::string path){
        if(!file::is_file(path)){
            std::string errort = "not find file '" + path+"'";
            log::err(errort);
            std::exit(STATUS_OPERATION_ERROR);
        }
        std::string file_data = file::read_file<std::string>(path);
        std::vector<vars> kvar = kconf(file_data, {});
        client_configure retconf;
        bool servername = false;
        bool serverport = false;
        bool mysessionkey = false;

        bool esexit= false;
        uint8_t opts[10] = {};
        opts[0] = 0x01;
        opts[2] = 0xFF;
        opts[9] = mazor_code;
        retconf.options = req10_t(opts);
        
        for (auto var : kvar){
            if(var.name == "host"){
                if (var.valib.type == VSID::stringV){
                    retconf.hostname = var.valib.value;
                    servername = true;
                }
                else{
                    log::err("var 'host' need type STRING");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            if(var.name == "port"){
                if (var.valib.type == VSID::intV){
                    retconf.port = atoi(var.valib.value.c_str());
                    serverport = true;
                }
                else{
                    log::err("var 'port' need type INT");
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }

            if(var.name == "sey"){
                char raw[20] = {0};
                if(var.valib.value.size() == 20) {
                    memcpy(raw, var.valib.value.c_str(), 20);
                } else {
                    log::err("invalid session key");
                    std::exit(STATUS_OPERATION_ERROR);
                }
                retconf.sey = sey_t(raw);
                mysessionkey = true;
                
     
            }
        }      
        
        if(!servername){
            log::err("no find option in config 'host'");
            esexit = true;
        }
        if(!serverport){
            log::err("no find option in config 'port'");
            esexit = true;
        }
        if(!mysessionkey){
            log::err("no find option in config 'sey'");
            esexit = true;
        }
        return retconf;
    }

}

std::string generate_random_key() {
 
    constexpr char charset[] = 
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    constexpr size_t charset_size = sizeof(charset) - 1;
    
   
    std::random_device rd;
    std::uniform_int_distribution<size_t> dist(0, charset_size - 1);
    

    std::string key;
    key.reserve(20);
    
    for (size_t i = 0; i < 20; ++i) {
        key += charset[dist(rd)];
    }
    
    return key;
}

std::string expand_user_path(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }
    
    const char* home = getenv("HOME");
    if (!home) {
     
        home = getpwuid(getuid())->pw_dir;
    }
    
    return home + path.substr(1);
}