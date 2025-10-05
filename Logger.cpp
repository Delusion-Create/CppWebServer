#include"Logger.h"
#include<iostream>

void Log(std::string level, std::string message, std::string file_name, int line)
{
    std::cout<<"["<<level<<"]["<<time(nullptr)<<"]["<<message<<"]["<<file_name<<"]["<<line<<"]"<<std::endl;
}