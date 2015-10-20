// author   : jianbin.hong.cn@gmail.com
// date     : 2015/10/19

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include "utf8.h"
#include "config4cpp/Configuration.h"
using namespace config4cpp;
using namespace std;

// field index in the rule file
const int RULE_CATALOG_ORI_INDEX    = 0;
const int RULE_CATALOG_NAME_INDEX   = 2;
const int RULE_CATALOG_LEVEL_INDEX  = 3;
// field index in the process file
const int PROC_CATALOG_NAME_INDEX   = 1;

// the basic catalog info element 
struct catalog_info_struct{
    string name;
    string info;
}CA_INFO;

struct catalog_res_struct{
    string catalog;
    vector<catalog_info_struct> ele;
}CA_STRUCT;

// REF:
// http://stackoverflow.com/questions/402283/stdwstring-vs-stdstring
// http://utfcpp.sourceforge.net/
// http://www.zedwood.com/article/cpp-utf8-char-to-codepoint

// redis key to get second class
std::string rKey_second_class(const std::string& key){
    std::string local_key = key;
    return "KEY::SECOND-CLASS::-" + key;
}

// redis key to get class count
std::string rKey_Count(const std::string& key){
    std::string local_key = key;
    return "KEY::COUNT-" + key;
}

inline bool file_exists(const std::string& name){
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

// convert int into unicode string
std::string utf8chr(int cp)
{
        char c[5]={ 0x00,0x00,0x00,0x00,0x00 };
        if     (cp<=0x7F) { c[0] = cp;  }
        else if(cp<=0x7FF) { c[0] = (cp>>6)+192; c[1] = (cp&63)+128; }
        else if(0xd800<=cp && cp<=0xdfff) {} //invalid block of utf8
        else if(cp<=0xFFFF) { c[0] = (cp>>12)+224; c[1]= ((cp>>6)&63)+128; c[2]=(cp&63)+128; }
        else if(cp<=0x10FFFF) { c[0] = (cp>>18)+240; c[1] = ((cp>>12)&63)+128; c[2] = ((cp>>6)&63)+128; c[3]=(cp&63)+128; }
        return std::string(c);
}

namespace nlpUtil{

    void utf8_substr(const string& str,
                const int start, 
                const int length,
                string& res);

    void parse(const std::string& name,
                const int length,
                vector<string>& res){ 
        int txt_length = utf8::distance(name.begin(), name.end());  
        for(int i=0; i<txt_length; i++){
            string local_str = "";
            if(i + length <= txt_length){
                utf8_substr(name, i, length, local_str);
                // cout << "i : " << i << "length : " << length << endl;
                // cout << local_str << endl;
                res.push_back(local_str);
            }
        }
        if (length + 1 < txt_length){
            parse(name, length + 1, res);   
        }
    }

    void utf8_substr(const string& str,
                const int start, 
                const int length,
                string& res){
        char* chr_str   = (char *) str.c_str(); 
        int txt_length  = utf8::distance(str.begin(), str.end());
        if(length > txt_length){
            cout << "[ERROR][Namespace::ulpUtil][Function::utf8_substr] - substring length is longer than the origin string. Original String: " << str << " substring length : " << length << endl;
            exit(-1);
        }

        if(start + length > txt_length){
            cout << "[ERROR][Namespace::ulpUtil][Function::utf8_substr] - substring length start_index + length is long than the origin string. Original String: " 
                << str << "; substring length : " << length 
                << "; start_index : " << start << endl;
            exit(-1);
        }
        char* chr_start = chr_str; 
        char* chr_end   = chr_start + strlen(chr_str) + 1;
        int sta_length  = 0;
        int cut_length  = 0;
        do {
            uint32_t code = utf8::next(chr_start, chr_end);
            if (code == 0)
                continue;
            if (sta_length < start){
                sta_length++;
                continue;
            }
            // std::cout << code << " : " << utf8chr(code) << std::endl;
            string inner_str    = utf8chr(code);
            string pre_str      = res;
            res                 = pre_str + inner_str;
            cut_length++;
        }while(chr_start < chr_end && cut_length < length);
    }
}

// catalog label process
// PARAM:
//      - @name : the name we will process
//      - @name2catalog: feature to catalog mapping 
//      - @name2level: feature to level mapping
//      - @res: the process result info
void catalog_label_process(const string& name,
                            map<string, string>& name2catalog,
                            map<string, string>& name2level,
                            catalog_res_struct &res)
{
}

int main(int argc, char* argv){

    Configuration       *cfg        = Configuration::create();
    const char          *scope      = "";
    const char          *configFile = "conf.cfg";
    const char          *inputFile;
    const char          *outputFile;
    const char          *ruleFile;

    try{
        cfg->parse(configFile);
        inputFile       = cfg -> lookupString(scope, "INPUT_FILE");
        outputFile      = cfg -> lookupString(scope, "OUTPUT_FILE");
        ruleFile        = cfg -> lookupString(scope, "RULE_FILE");
    }catch(const ConfigurationException & ex){
        std::cerr << ex.c_str() << std::endl; 
        cfg -> destroy();
        return 1;
    }

    // parse the rule file into two map variable
    map<string, int> key2level;
    map<string, string> key2catalog;

    std::ifstream inFileObj;
    bool b_file_value = file_exists(ruleFile);
    if(!b_file_value){
        cout << "[ERROR][Function::main] - RuleFile is not exist! - file path: " << ruleFile << endl;
        exit(-1);
    }

    inFileObj.open(ruleFile, std::ios::in);
    if(! inFileObj.is_open()){
        cout << "[ERROR][Function::main] - Could not open RuleFile - file path: " << ruleFile << endl;
        exit(-1);
    }
    std::string line;
    while(std::getline(inFileObj, line)){
        istringstream iss(line);
        vector<string> tokens;
        copy(istream_iterator<string>(iss),
                istream_iterator<string>(),
                back_inserter(tokens));
        string key = tokens[RULE_CATALOG_ORI_INDEX];

        key2catalog[key]    = tokens[RULE_CATALOG_NAME_INDEX];
        key2level[key]      = std::atoi(tokens[RULE_CATALOG_LEVEL_INDEX].c_str());
    }

    inFileObj.close(); // close the rule file

    // open the input file
    b_file_value = file_exists(inputFile);
    if(!b_file_value){
        cout << "[ERROR][Function::main] - inputFile is not exist! - file path: " << inputFile << endl;
        exit(-1);
    }

    std::ifstream inputFileObj;
    inputFileObj.open(inputFile, std::ios::in);
    if(!inputFileObj.is_open()){
        cerr << "[ERROR][Function::main] - Could not open inputFile - file path: " << inputFile << endl;
        exit(-1);
    }

    while(std::getline(inputFileObj, line)){
        istringstream iss(line);
        vector<string> tokens;
        copy(istream_iterator<string>(iss),
                istream_iterator<string>(),
                back_inserter(tokens));
    }

    inputFileObj.close();

    // utf8 test part
    std::string man = "算了解放i";
    // std::cout << man << std::endl;
    
    int length = utf8::distance(man.begin(), man.end());
    std::cout << "UTF8-lib string length: " << length << std::endl;

    std::string local_str = rKey_second_class(man);
    std::cout << local_str << std::endl;
    char* str   = (char*) man.c_str();
    char* str_i = str;
    char* end   = str + strlen(str) + 1;

//    do {
//        uint32_t code = utf8::next(str_i, end);
//        if (code == 0)
//            continue;
//        std::cout << code << " : " << utf8chr(code) << std::endl;
//    }while(str_i < end);

    vector<string> v_str;
    nlpUtil::parse(man, 2, v_str);
    for(vector<string>::iterator it=v_str.begin(); it<v_str.end(); it++){
        cout << *it << endl;
    }
    return 0;
}

