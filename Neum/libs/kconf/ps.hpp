class kairrer_conf_tpr {
    std::vector<TOKENS> tokens;
    urwerer& state;
    size_t vap = 0;
    urwerer::LOG log;
    const TOKENS& lift() { return tokens[vap--]; } 
    const TOKENS& cret() { return tokens[vap++]; }
    const TOKENS& lixt() const { return tokens[vap]; } 
    TOKENS& lixtCret(int cret) {
        int newpin = vap;
        return tokens[newpin+cret];
    } 
    bool vaZexc() const { return vap >= tokens.size(); } 
    bool vadap(TSID type) const { return !vaZexc() && lixt().type == type; }
    bool dapin(TSID type) { 
        if (vadap(type)) { cret(); return true; }
        return false;
    }
    bool daplift(TSID type){ 
        if (vadap(type)) return true; 
        
        lift();
        return false;
    }
    bool esValue(){ return vadap(INT) || vadap(SI) || vadap(STRING) || vadap(S) || vadap(SS) || vadap(GE) || vadap(VA); }

    void skipTAB(){while(vadap(TSID::TAB))cret();} 
    void skipLINE(){while(!dapin(TSID::INGOR) &&!vaZexc())cret();}
    void echoLIXT(){ std::cout<<"'"<<lixt().value<<"'"<<std::endl;} 
    int  getLINE() {
        int newpin = vap;
        int lines = 1;
        while(newpin != 0){
            if(tokens[newpin].type==TSID::INGOR) lines++;
            newpin--;
        }
        return lines;
    }
    int  getSINE() {
        int newpin = vap;
        int sims = 0;
        while (newpin != 0 && tokens[newpin].type!=INGOR){
            sims ++;
            newpin -- ;
        }
        return sims;
    }
    std::string line_pin(){return "["+std::to_string(getLINE())+"|"+std::to_string(getSINE())+"]";}

    std::string lixtS() {
        std::string value;
        cret();
        while (!vaZexc() && !vadap(TSID::S)) {
            if (dapin(TSID::LINK)) {
                vars varlink= lixtLINK();
                if(varlink.valib.type!=VSID::stringV&&varlink.valib.type!=VSID::intV) log.krr("gat ret var ' <- ten "+line_pin());
                
                else value += varlink.valib.value; 
            }
            else value += cret().value;
        };
        if (!dapin(TSID::S)) log.fkr("S", "un drit et -> " + SIMS::get_sname_from_name(TSID::S)+line_pin());
        return value;
    }
    std::string lixtSS() { 
        std::string value;
        cret();
        while (!vaZexc() && !vadap(TSID::SS)) {
            if (dapin(TSID::LINK)) {
                vars varlink= lixtLINK();
                if(varlink.valib.type!=VSID::stringV&&varlink.valib.type!=VSID::intV) log.krr("gat ret var \" <- ten "+line_pin());
                else value += varlink.valib.value; 
            }
            else value += cret().value;
        }
        if (!dapin(TSID::SS)) log.fkr("SS","un drit et -> " + SIMS::get_sname_from_name(TSID::SS)+" "+lixt().value+line_pin());
        return value;
    }

    vars        lixtLINK(){
        vars retvar;
        std::string varname =lixtSTRINT();
        auto var = state.getVariable(varname);
        retvar.name=varname;
        if (var){
            retvar.valib.type=var->valib.type;
            retvar.valib.value=var->valib.value;
        } 
        else{
            retvar.valib.type = VSID::nullV;
            retvar.valib.value = NULL_STR;
            log.krr("fo vanih hamu var: @" + varname + " " + line_pin());
        }
        return retvar;
    }
    std::string lixtINT(){
        std::string senInt;
        if (dapin(TSID::SI)) senInt+=SIMS::get_sim_from_name(TSID::SI);
        while(!vaZexc() && vadap(TSID::INT))senInt+=cret().value;
        
        return senInt;
    }
    std::string lixtSTRING() {
        std::string stringData;
        while (vap < tokens.size() && vadap(TSID::STRING))stringData+= cret().value;    
        return stringData;
    }
    std::string lixtSTRINT(){
        std::string stringData;
        while (vap < tokens.size() && (vadap(TSID::STRING) ||vadap(TSID::INT)))stringData+= cret().value;    
        return stringData;
    }
    
    
    value       lixtVALUE(value &vlib){
        if(vadap(TSID::S)){
            vlib.type=VSID::stringV;
            vlib.value=lixtS();
        }
        else if(vadap(TSID::SS)){
            vlib.type=VSID::stringV;
            vlib.value=lixtSS();
        }
        else if( vadap(TSID::INT) ||  vadap(TSID::SI) && lixtCret(1).type != TSID::CU ){
            vlib.type=VSID::intV;
            vlib.value=lixtINT();
        }
        
        else if (vadap(TSID::STRING))  vlib=varWer().valib;
        

        else log.fkr("VALUE","fat ret ame value et -> '"+lixt().value+"' "+line_pin());        
        return vlib;
    }
    
    

    vars        varWer(){
        vars varlib;
        std::string vname = lixtSTRINT();
        varlib.setName(vname);
        auto nullvar_false = [&](){
            vars nvar;
            nvar.valib.type  = nullV;
            nvar.valib.value = NULL_STR;
            return nvar;
        };
       

     
        
        skipTAB();


        /* : */if     (vadap(BEDTAK)){
            cret();
            skipTAB();
            
            lixtVALUE(varlib.valib);
            state.addVariable(varlib);
        }

        else{
            auto varh = state.getVariable(vname);
       
            if(varh){
                varlib.valib = varh -> valib;
                
            }
            else {
                varlib.setName(vname);
                varlib.valib.type  = nullV;
                varlib.valib.value = NULL_STR;

                
            };
        }            
        return varlib;
    }
    
    

    public:
    kairrer_conf_tpr(const std::vector<TOKENS>& tokens, urwerer& state) : 
        tokens(tokens), state(state), log(state) {}

    std::vector<vars> prun(std::vector<vars> eninv) { // PARSE AND RUN
        for(vars ev:eninv) state.addVariable(ev);
        std::vector<vars> retvars;


        while (!vaZexc()) {
            try {
                //var     
                     if (esValue()) {value enle; lixtVALUE(enle);}
                // else if (esValue()){}
                
        
                //comes
                else if (dapin(TSID::DREB))   {skipLINE(); }
                else if (dapin(TSID::DYR) && lift().type==TSID::DYR) skipLINE();
                //excutes
                //skipis
                else cret();

            } catch (const std::exception& e) {
                log.fkr("PARS","token " + std::to_string(vap) + ": " + e.what() + "\n "+line_pin());
            }
        }
        
        retvars = state.getVariables(); 
        return retvars;
    }

};
