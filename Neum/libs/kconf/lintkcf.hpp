#include "bed.hpp"
std::vector<vars> kconf(std::string code,std::vector<std::string> argm){
    std::vector<TOKENS> tokens = tokenizer(code);
    urwerer state;
    kairrer_conf_tpr ktpr(tokens, state);
    std::vector<vars> varsarg;
    vars AP;
    AP.name = "AP";
    AP.valib.type = VSID::intV;
    AP.valib.value = AP__KRR__CONF;
    varsarg.push_back(AP);

    vars BIN;
    BIN.name = "BIN";
    BIN.valib.type = VSID::intV;
    BIN.valib.value = BIN__KRR__CONF;
    varsarg.push_back(BIN);

    vars FO;
    FO.name = "FO";
    FO.valib.type = VSID::intV;
    FO.valib.value = FO__KRR__CONF;
    varsarg.push_back(FO);
    return ktpr.prun(varsarg);
}