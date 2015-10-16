// author : jianbin.hong.cn@gmail.com
// date   : 2015/10/14
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <iterator>
#include <fstream>
#include <sstream>
#include "mulPThread.h"

using namespace std;

// declare part

inline bool file_exists(const std::string& name);
struct arg_struct{
    string inputFilePath;
    string outputFilePath;
};

// end of declare part

namespace patch
{
        template < typename T > std::string to_string( const T& n )
        {
            std::ostringstream stm;
            stm << n;
            return stm.str();
        }
}

inline bool file_exists(const std::string& name){
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

void* func_example(void *argv){
    struct arg_struct *local_argv = (struct arg_struct *)argv;
    string inputFilePath = local_argv -> inputFilePath;
    string outputFilePath = local_argv -> outputFilePath;
    // output each line of inputFile into output file
    bool b_file_value = file_exists(inputFilePath);
    if(!b_file_value){
        cout << "[ERROR][Function::func_example] - inputFilePath not exist - file path: " << inputFilePath <<endl;
        exit(-1); // if the inputFilePath not exist, then exist the program
    }

    std::ifstream inputFile;
    std::ofstream outputFile;

    inputFile.open(inputFilePath.c_str(), std::ios::in);
    outputFile.open(outputFilePath.c_str(), std::ios::out);

    if(! inputFile.is_open()){
        cout << "[ERROR][Function::func_example] - Could not open inputFilePath - file path: " << inputFilePath << endl;
        exit(-1);
    }
    string line;
    while(getline(inputFile, line)){
        outputFile << line << endl;
    }
    inputFile.close();
    outputFile.close();
}
            
void func_multiPThread(const std::string& inputFilePath,
                        const std::string& outputFilePath,
                        const std::string& tmpInDir,
                        const std::string& tmpOutDir,
                        const std::string& part_name,
                        void* (*func)(void *argv)){
    // check whether input file is exist  
    bool b_file_value = file_exists(inputFilePath);
    if(!b_file_value){
        cout << "[ERROR][Function::func_multiPThread] - inputFilePath not exist - file path: " << inputFilePath <<endl;
        exit(-1); // if the inputFilePath not exist, then exist the program
    }

    // count total line numbers of the input file
    std::size_t line_num = 0;
    std::ifstream inputFile;
    inputFile.open(inputFilePath.c_str(), std::ios::in);
    if ( ! inputFile.is_open() ){
        cout << "[ERROR][Function::func_multiPThread] - Could not open inputFilePath - file path: " << inputFilePath << endl;
        exit(-1);
    }
    line_num = std::count(std::istreambuf_iterator<char>(inputFile), 
                        std::istreambuf_iterator<char>(), '\n');
    inputFile.close();
    cout << "[INFO][PROCESSING::INFO] InputFile line count : " << line_num << endl;
    
    int sub_file_line = line_num / NTHREADS;  
    cout << "[INFO][PROCESSING::INFO] Each sub file line number : " << sub_file_line << endl;
    // split large file into smaller sub file
    inputFile.open(inputFilePath.c_str(), std::ios::in);

    for (int i=0; i<NTHREADS; i++)
    {
        std::ofstream outputFile;
        std::string local_file_path;
        std::string local_file_name;

        local_file_name = part_name + patch::to_string(i);
        local_file_path = tmpInDir + local_file_name;

        outputFile.open(local_file_path.c_str(), std::ios::out);
        if (!outputFile.is_open()){
            cout << "[ERROR][Function::func_multiPThread] - Could not open outputFilePath - file path: " << inputFilePath << endl; 
            exit(-1);
        }
        std::size_t local_line_count = 1;

        std::string line; 
        for(int j=0; j < sub_file_line; j++){
            while(local_line_count < sub_file_line && std::getline(inputFile, line)){
                outputFile << line << endl;
                local_line_count++;
            }
        }

        if(i == NTHREADS - 1){
            while(std::getline(inputFile, line)){
                outputFile << line << endl;
            }
        }
        outputFile.close();
    }

    cout << "[INFO][PROCESS::INFO] Sucess in spliting Large File into sub smaller file !" << endl;

    // run each sub file under given function 
    pthread_t thread_id[NTHREADS];
    arg_struct arg_id[NTHREADS];
    for (int i = 0; i<NTHREADS; i++){
        string local_file_input;
        string local_file_output;
        string local_file_name;

        local_file_name = tmpInDir + part_name;
        arg_id[i].inputFilePath = local_file_name + patch::to_string(i);
        local_file_name = tmpOutDir + part_name;
        arg_id[i].outputFilePath = local_file_name + patch::to_string(i);
        
        pthread_create(&thread_id[i], NULL, func, (void *)&arg_id[i]);
    }
    
    for (int i = 0; i < NTHREADS; i++){
        pthread_join(thread_id[i], NULL);
    }
}
            
int main(){
    func_multiPThread("/home/tuan/data/map.selected",
                    "",
                    "/home/tuan/users/jianbin/code/tmpIn/",
                    "/home/tuan/users/jianbin/code/tmpOut/",
                    "part",
                    &func_example);
    return 0;
}
