enum VSID{ stringV, intV, nullV };

std::string get_sname_from_vname(VSID VNAME){
    std::string return_vname="<fater>";
    
    if     (VNAME == stringV)  return_vname = "<stringV>";
    else if(VNAME == intV)     return_vname = "<intV>";
    else if(VNAME == nullV)    return_vname = "<nullV>";

    return return_vname;
}


struct vars;

struct value {
    VSID type;
    std::optional<std::string> vname;
    std::string value;

    bool operator==(const struct value& other) const {
        return type == other.type && value == other.value;
    }
    
};

struct vars {
    std::string name;
    value valib;
    void setName(std::string vname) {
        name = vname;
        valib.vname = vname; 
    }
    bool operator==(const vars& other) const {
        return name == other.name && valib == other.valib;
    }
};


class urwerer {
    std::unordered_map<std::string, vars> variables;
   
public:
    class LOG {
        urwerer& lixtwerrer;
        public:
        LOG(urwerer& p) : lixtwerrer(p) {}
        void krr(std::string v);
        void cev(std::string v);
        void fkr(std::string ut, std::string v);
    };
    urwerer() : logger(*this) {} 
    LOG logger;
    void addVariable(const vars& var) {
        variables[var.name] = var;
    }
    std::optional<vars> getVariable(const std::string& name) const {
        auto it = variables.find(name);
        return it != variables.end() ? std::optional(it->second) : std::nullopt;
    }
    std::vector<vars> getVariables(){
        std::vector<vars> variablis;
        for(auto var : variables){
            std::pair<std::string,vars> vare=var;
            variablis.push_back(vare.second);
        }
        return variablis;
    }
    bool removeVariable(const std::string& name) {
        auto  dwar= getVariable(name);

        return variables.erase(name) > 0;
    }
    
        
    
};

void urwerer::LOG::krr(std::string v) {
    auto vakair = lixtwerrer.getVariable("givKair");
    if (vakair) {
        if (vakair->valib.value == FO__KRR__CONF) return;
    }
    std::cerr << red << "|[KAIR]" << v << reset << std::endl;
}

void urwerer::LOG::cev(std::string v) {
    auto vacev = lixtwerrer.getVariable("givCev");
    if (vacev) {
        if (vacev->valib.value == FO__KRR__CONF) return;
    }
    std::cerr << yellow << "|[CEV] " << v << reset << std::endl;
}

void urwerer::LOG::fkr(std::string ut, std::string v) {
    krr("|[" + ut + "] " + v);
    exit(1);
}

#include "ps.hpp"
