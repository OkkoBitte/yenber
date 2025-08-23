#include <includelib.hpp>
#include <center.hpp> 
#include "Ycenter.hpp"
struct type_y {
    static const inline uint8_t text_t = 0x01;
    static const inline uint8_t file_t = 0x02;
    static const inline uint8_t shll_t = 0x03;
};

enum type_f {
    download,
    upload 
};
enum typeACML{
    nullCML, clearCML, exitCML, messageCML, fileCML, shellCML
};
struct actionsCml{
    typeACML action_type = {typeACML::nullCML}; // type
    std::string arg1 = {}; // to who?
    std::string arg2 = {}; // reserv data
    std::string arg3 = {}; // reserv data


};



namespace fs = std::filesystem;

class fileManager {
    std::atomic<int> fsr{0};        
    std::atomic<int> chunks_received{0};
    uint64_t fullsize = 0;     
    std::ofstream file_stream; 
    uint64_t total_chunks = 0;
    std::string filename;
    std::mutex file_mutex;
    type_f ftype;

public:
    fileManager(std::string filename_arg, type_f ftype_arg) : filename(filename_arg), ftype(ftype_arg) {}
    
    void startTransfer(uint64_t chunks_total) {
        std::lock_guard<std::mutex> lock(file_mutex);
        total_chunks = chunks_total;
        file_stream.open(filename, std::ios::binary);
        if (!file_stream) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
    }
    
void getFile(uint64_t chunk_num, uint64_t chunks_total, std::vector<uint8_t> data) {
    std::lock_guard<std::mutex> lock(file_mutex);
    
   
    if (total_chunks == 0) {
        total_chunks = chunks_total;
        if (file_stream.is_open()) file_stream.close();
        file_stream.open(expand_user_path("~/yenber/downloads/"+filename), std::ios::binary);
        if (!file_stream) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        std::cout << "Starting file transfer: " << filename 
                 << " (" << total_chunks << " chunks)" << std::endl;
    }
    
    else if (chunk_num == 1 && total_chunks != chunks_total) {
        std::cerr << "Warning: chunk #1 has different total chunks count: " 
                 << chunks_total << " vs " << total_chunks << std::endl;
        total_chunks = std::max(total_chunks, chunks_total);
    }

    file_stream.write(reinterpret_cast<const char*>(data.data()), data.size());
    fsr += data.size();
    chunks_received++;
    
    int percent = 0;
    if (total_chunks > 0) {
        percent = (100 * chunks_received) / total_chunks;
    }
    
    std::cout << "\rReceiving " << filename << ": " 
             << chunks_received << "/" << total_chunks
             << " chunks (" << percent << "%)" << std::flush;
    
    if (chunks_received == total_chunks) {
        file_stream.close();
        std::cout << "\nFile transfer complete: " << filename 
                 << " (" << fsr << " bytes)" << std::endl;
    }
}
    void sendFile(const std::string& recipient_sey, clientManager& cm, const fs::path& filepath) {
    
        std::thread sendth([recipient_sey, &cm, filepath]() { 
            constexpr size_t CHUNK_SIZE = 60'000;
            
         
            std::ifstream file(filepath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << filepath.string() << std::endl;
                return;
            }

            const uint64_t total_size = file.tellg();
            file.seekg(0);
            const std::string filename = filepath.filename().string();
            const uint8_t filename_size = static_cast<uint8_t>(filename.size());
            
            const uint64_t total_chunks = (total_size + CHUNK_SIZE - 1) / CHUNK_SIZE;
            uint64_t current_chunk = 0;
            int last_chunk = 0;
            while (file && current_chunk < total_chunks) {
                const auto remaining = total_size - file.tellg();
                const auto chunk_size = std::min(CHUNK_SIZE, static_cast<size_t>(remaining));
                
                std::vector<uint8_t> chunk(chunk_size);
                file.read(reinterpret_cast<char*>(chunk.data()), chunk_size);
                
                if (file.fail() && !file.eof()) {
                    std::cerr << "File read error" << std::endl;
                    break;
                }
                
                current_chunk++;

                std::vector<uint8_t> packet;
                packet.insert(packet.end(), recipient_sey.begin(), recipient_sey.end());
                packet.push_back(type_y::file_t);
                packet.push_back(filename_size);
                packet.insert(packet.end(), filename.begin(), filename.end());

                uint64_t be_current = htobe64(current_chunk);
                packet.insert(packet.end(), 
                            reinterpret_cast<uint8_t*>(&be_current),
                            reinterpret_cast<uint8_t*>(&be_current) + 8);

                uint64_t be_total = htobe64(total_chunks);
                packet.insert(packet.end(),
                            reinterpret_cast<uint8_t*>(&be_total),
                            reinterpret_cast<uint8_t*>(&be_total) + 8);

                packet.insert(packet.end(), chunk.begin(), chunk.end());
                

 
                
                cm.sendData(std::move(packet));
                int percent = 0;
                if (total_chunks > 0) {
                    percent = (100 * current_chunk) / total_chunks;
                } else {
                    percent = 100;
                }
                std::cout << "\rSending " << filename << ": " 
                        << current_chunk << "/" << total_chunks
                        << " chunks (" << percent << "%)";
            
             
           
            }
            
            std::cout << "\nFile transfer complete: " << filename << std::endl;
        });

        sendth.detach();
    }

    ~fileManager() {
        if (file_stream.is_open()) {
            file_stream.close();
        }
    }
};

class myApp : public clientManager {
public:
    myApp(yClientConfigController::retConf& conf) : 
        clientManager(conf.cc), 
        host(std::make_unique<hostManager>(static_cast<clientManager*>(this))), 
        confucen(conf.cc) {
        
        setvar = conf.all_var;
        std::cout << "|[SEY][" << cyan << std::string(confucen.sey.sey_main, 20) << reset << "]" << std::endl;
    }

    void newCml() {
        std::cout << "|[" << cyan << std::string(confucen.sey.sey_main, 20) << reset << "]> ";
    }

    void connected() override {
        
        bool t = true;
        while(t) {
            newCml();
            std::string fulltext;
            std::getline(std::cin, fulltext);
            
            std::vector<std::string> words;
            words = splitIntoWords(fulltext);
            if(words.empty()) continue;
            
            actionsCml action;
            std::string firecode = words[0];
            //parse
          
            if(spec_code.find(firecode) == spec_code.end()){
                std::unique_lock<std::mutex> lock(mutexmy);
                if (setvar.find(firecode)!= setvar.end()){
                    std::vector<std::string> nwords = splitIntoWords(setvar.find(firecode)->second);
                    firecode = nwords[0];
                    for(size_t i = 1; i < nwords.size(); ++i){
                        words.insert(words.begin()+i,nwords[i]);
                    }
                }
            }
            if(firecode== "|exit") {
                action.action_type = typeACML::exitCML;
            }
            if(firecode == "|clear") {
                action.action_type = typeACML::clearCML;
            }
            else if(firecode == "|msg") {
                if(words.size() > 1) {
                    
                    if(words[1].size() != 20) {
                        std::cout << "Invalid recipient sey (must be 20 bytes)" << std::endl;
                        continue;
                    }
                    
                    std::string message_text;
                    for(size_t i = 2; i < words.size(); ++i) {
                        if(i > 2) message_text += " ";
                        message_text += words[i];
                    }
                    action.action_type = typeACML::messageCML;
                    action.arg1 = words[1];
                    action.arg2 = message_text;
                }
            }
            else if(firecode == "|file") {
                if(words.size() < 3) {
                    std::cout << "Usage: file <recipient_sey> <filepath>" << std::endl;
                    continue;
                }
                
                if(words[1].size() != 20) {
                    std::cout << "Invalid recipient sey (must be 20 bytes)" << std::endl;
                    continue;
                }
                
                std::string clean_path = words[2];
                if(!clean_path.empty() && (clean_path.front() == '"' || clean_path.front() == '\'')) {
                    clean_path = clean_path.substr(1, clean_path.size() - 2);
                }
                
                fs::path filepath(clean_path);
                if(!fs::exists(filepath)) {
                    std::cout << "File not found: " << filepath << std::endl;
                    continue;
                }
                std::cout<< "File "<<filepath<<" is exists!"<<std::endl;

                action.action_type = typeACML::fileCML;
                action.arg1 = words[1];
                action.arg2 = filepath;
                
  
            }
            else if(firecode == "|shell"){
                if(words[1].size() != 20) {
                    std::cout << "Invalid recipient sey (must be 20 bytes)" << std::endl;
                    continue;
                }
                
                std::string full_code;
                for(size_t i = 2; i < words.size(); ++i) {
                    if(i > 2) full_code += " ";
                    full_code += words[i];
                }
                action.action_type = typeACML::shellCML;
                action.arg1 = words[1];
                action.arg2 = full_code;
            }
            else if(firecode == "|set"){
                if(words[1].size() >= 1 && words[2].size() >= 1){
                    std::string fullwr;
                    for(size_t i = 2; i < words.size(); ++i) {
                        if(i > 2) fullwr += " ";
                        fullwr += words[i];
                    }

                    setvar[words[1]] = fullwr;
                    
                }
                else std::cout<<"Err for set"<<std::endl;
            }
            else {
                std::cout << "\n\nUnknown command: " << firecode << std::endl;
                std::cout << "Available commands:\n\t|msg <sey> <text> -- send text\n\t|file <sey> <path> -- send file\n\t|exit -- quit\n\t|clear -- clear chat\n\t|set <link> <text> -- variables\n\t|shell <sey> <command> -- run command\n\n";
            }
            
            cmlActivate(action);
        }

    }


    void getData(std::vector<uint8_t> data) override {
 
        if(data.size() < 21) return;
        
        std::string sender_sey(data.begin(), data.begin() + 20);
        uint8_t data_type = data[20];
        
        if(data_type == type_y::text_t) { 
            std::string message(data.begin() + 21, data.end());
            std::cout << "\n|[" << magenta << sender_sey << reset << "]: " << message << std::endl;
            return;
        }
        else if(data_type == type_y::file_t) {
            std::unique_lock<std::mutex> lock(mutexmy);
            if (setvar.find("getFile")!= setvar.end()){
                if(setvar.find("getFile")->second != "1") return;
            }
            lock.unlock();
            if(data.size() < 40) { 
                std::cerr << "Invalid file packet (too small)" << std::endl;
                return;
            }

            size_t offset = 21; 
            uint8_t filename_size = data[offset];
            offset += 1; 

            if(offset + filename_size + 16 > data.size()) {
                std::cerr << "Invalid file packet (filename too long or no data)" << std::endl;
                return;
            }

         
            std::string filename(data.begin() + offset, data.begin() + offset + filename_size);
            offset += filename_size; 

           
            uint64_t current_chunk, total_chunks;
            memcpy(&current_chunk, data.data() + offset, 8);
            memcpy(&total_chunks, data.data() + offset + 8, 8);
            current_chunk = be64toh(current_chunk);
            total_chunks = be64toh(total_chunks);
            offset += 16; 

 
            size_t data_size = data.size() - offset;
            const uint8_t* file_data = data.data() + offset;


            
            try {
                auto it = downloads.find(filename);
                if (it == downloads.end()) {
                    it = downloads.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(filename),
                        std::forward_as_tuple(filename, type_f::download)
                    ).first;
                }
                it->second.getFile(current_chunk, total_chunks,std::vector<uint8_t>(file_data, file_data + data_size));
                
                if (current_chunk == total_chunks) {
                    downloads.erase(filename);
                }
            } catch (const std::exception& e) {
                std::cerr << "File transfer error: " << e.what() << std::endl;
                downloads.erase(filename);
            }
        }
        else if(data_type == type_y::shll_t){
            std::unique_lock<std::mutex> lock(mutexmy);
            if (setvar.find("getShell")!= setvar.end()){
                if(setvar.find("getShell")->second != "1") return;
            }
            std::string shcode(data.begin() + 21, data.end());
            std::cout << "\n|[" << magenta << sender_sey << reset << "]$ " << shcode << std::endl;
            std::array<char, 128> buffer;
            std::string result;
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(shcode.c_str(), "r"), pclose);
            if (!pipe) result = "Command execution failed: " + shcode;
            while (fgets(buffer.data(), buffer.size(), pipe.get())) result += buffer.data();
            sendMessage(sender_sey,result,1);
        }
    }
    void closeConnection() override {
        std::cout << "Connection closed" << std::endl;
        std::exit(0);
    }

private:
    std::unique_ptr<hostManager> host;
    client_configure confucen;
    std::map<std::string, fileManager> downloads;
    std::mutex mutexmy;
    std::map<std::string, std::string> setvar;
    const std::set<std::string> spec_code = {"|exit","|clear","|msg","|file","|shell","|set"};
    void sendMessage(std::string hamu, std::string text, int who){

        std::vector<uint8_t> data;
        data.insert(data.end(), hamu.begin(), hamu.end());
        if(who == 1){data.push_back(type_y::text_t);}
        else if (who == 2){
            data.push_back(type_y::shll_t);
        }
        data.insert(data.end(), text.begin(), text.end());
        
        sendData(data);

    }

    void cmlActivate (actionsCml act){
    
        if(act.action_type == typeACML::clearCML){
            std::system("clear");
        }
        else if(act.action_type == typeACML::exitCML){
            closeConnection();

        }   
        else if(act.action_type == typeACML::messageCML){
            sendMessage(act.arg1,act.arg2,1);
        }
        else if(act.action_type == typeACML::shellCML){
            sendMessage(act.arg1,act.arg2,2);
        }
        else if(act.action_type == typeACML::fileCML){
            fileManager fm = fileManager(act.arg2,type_f::upload);
            fm.sendFile(act.arg1, *this, act.arg2);
        }
        

    }

    std::vector<std::string> splitIntoWords(const std::string& input) {
        std::vector<std::string> words;
        std::istringstream iss(input);
        std::string word;
        
        while(iss >> word) {
            words.push_back(word);
        }
        
        return words;
    }
};

int main(int argc, char* argv[]) {
    if (argc == 1){
        yClientConfigController::retConf cf;

        std::string rootdir = expand_user_path("~/yenber");
        std::string rootconf = "config.kr";
        std::string rootfiledir = "downloads";

        bool isroot = file::is_dir(rootdir);
        bool isfiledir = file::is_dir(rootdir + "/" + rootfiledir);
        bool isconf = file::is_file(rootdir + "/" + rootconf);

        std::string newsey = generate_random_key();
        std::string confdata = "confC:\""+confC+"\";\nhost:\"web-mbg.ru\";\nport:333;\nsey:\"" + newsey + "\"; \n\n# VARS \ngetFile:AP; // for accept and download file \ngetShell:AP; // for accept and run shell code \n\nexit:'|exit'; clear:'|clear'; msg:'|msg'; file:'|file'; shell:'|shell'; set:'|set';";


        if(isroot && isfiledir && isconf) {
            cf = yClientConfigController::getConfig(rootdir + "/" + rootconf);
            bool is_valide_conf = false;
            if(cf.all_var.find("confC") != cf.all_var.end()){
                if(cf.all_var.find("confC")->second == confC){
                    is_valide_conf=true;
                };
            }
            if(!is_valide_conf){
                log::err("you using bad conf");
                std::cout << "[Yenber]| you want to re-build config? y/n :";
                std::string yf;
                std::cin >> yf;
                 
                if(yf == "y" || yf == "Y") {
                    file::new_file(rootdir + "/" + rootconf, confdata);
                    log::def("please restarting this program");
                    return 0;
                }
                else {std::cout<<"abort"; std::exit(STATUS_OPERATION_ERROR);}
            }
        }
        else {
            log::warn("you not binding Yenber sandow");
            std::cout << "[Yenber]| you want to build? y/n :";
            std::string yf;
            std::cin >> yf;
            
            if(yf == "y" || yf == "Y") {
                try {
                    file::new_dir(rootdir);
                    file::new_dir(rootdir + "/" + rootfiledir);
                    file::new_file(rootdir + "/" + rootconf, confdata);

                    log::def("please restarting this program");
                    return 0;
                } catch (const std::exception& e) {
                    log::err("Failed to create directories: " + std::string(e.what()));
                    std::exit(STATUS_OPERATION_ERROR);
                }
            }
            else {
                log::err("aborted");
                std::exit(STATUS_OPERATION_ERROR);
            }
        }

        try {
            myApp app(cf);
            app.loop();
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }

    }
    else{
        std::string argm = argv[1];
        if(argm == "--help"){
            std::cout<<help1_client;
        }
        else if(argm == "--describe"){
            std::cout<<help2_client;
        }
    }
   
    return 0;
}
