#include "bed.hpp"
std::vector<vars> kconf(std::string code,std::vector<std::string> argm){
    std::vector<TOKENS> tokens = tokenizer(code);
    urwerer state;
    kairrer_conf_tpr ktpr(tokens, state);
    std::vector<vars> varsarg;
    std::string args;
    int argi=1;
    for(auto arg:argm){
        vars nwar;
        nwar.name="arg"+std::to_string(argi);
        nwar.valib.type=VSID::stringV;
        nwar.valib.value=arg;
        args+=' '+arg;
        varsarg.push_back(nwar);
        argi++;
    } 
    vars nware;
    nware.name="args";
    nware.valib.type=VSID::stringV;
    nware.valib.value=args;
    varsarg.push_back(nware);
    return ktpr.prun(varsarg);
}