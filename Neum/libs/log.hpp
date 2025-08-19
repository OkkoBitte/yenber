class log{

public:
    static void def(std::string text){
        std::cout << "LOG: "<< text<<std::endl;
    }
    static void warn(std::string text){
        std::cout << "WARN: "<<yellow<<text<<reset<<std::endl;
    }
    static void err(std::string text){
        std::cerr << "ERR: "<<red<<text<<reset<<std::endl;
    }

};

