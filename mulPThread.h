// author : jianbin.hong.cn@gmail.com
// date   : 2015/10/14

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <iostream>
#include <string.h>

#define NTHREADS 128
#define MAX_FILE_LINE 100000


void func_multiPThread(const std::string& inputFilePath,
                        const std::string& outputFilePath,
                        const std::string& tmpInDir,
                        const std::string& tmpOutDir,
                        const std::string& part_name,
                        void* (*func)(void *argv));
