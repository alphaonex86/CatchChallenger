#include "xxhash.h"
#include <iostream>
#include <chrono>

int main()
{
    unsigned long long int count=0;
    char inputdata[4096];

    auto start = std::chrono::high_resolution_clock::now();

    int loop=2000000;
int index=0;
while(index<loop)
{
        XXH32(inputdata,sizeof(inputdata),0);
        index++;
}
count+=loop;


    auto end = std::chrono::high_resolution_clock::now();
    const unsigned int ms=std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    std::cout << "Result: " << count << " in " << ms << "ms\nMean:" << (long long int)((double)count*1000/(double)ms) << "H/s" << std::endl;

    return 0;
}
