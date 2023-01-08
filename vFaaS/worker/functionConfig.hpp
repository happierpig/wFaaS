#include <iostream>
#include <vector>
#include <fstream>

using std::vector;
using std::cin;
using std::cout;
using std::fstream;

class functionConfiguration{
    public:
        int stack_size, heap_size, return_size;
        int argc;
        vector<int> argsv;

        functionConfiguration() = default;

        ~functionConfiguration() = default;

        void setFromFile(const char* path){
            cout << "Try read function configuration from " << path << std::endl;
            fstream inFile;
            inFile.open(path, std::ios::in);
            inFile >> stack_size >> heap_size >> return_size >> argc;
            argsv.resize(argc);
            for(int i = 0;i < argc;++i) inFile >> argsv[i];
            cout << "Read Configuration Finish." << std::endl; 
        }

        void setFromStdin(){
            cout << "Type STACK_SIZE HEAP_SIZE RETURN_SIZE ARGC and ARG_SIZE in order." << std::endl;
            cin >> stack_size >> heap_size >> return_size >> argc;
            for(int i = 0; i < argc; ++i){
                int tmp;
                cin >> tmp;
                argsv.push_back(tmp);
            }
            cout << "Configure successfully." << std::endl;
        }

        void dumpIntoFile(){
            cout << "Dump the function Configuration into workerspace/func.config" << std::endl;
            fstream outFile;
            outFile.open("func.config", std::ios::out);
            outFile << stack_size << ' ' << heap_size << ' ' << return_size << ' ' << argc << ' ';
            for(int i = 0;i < argc; ++i) outFile << argsv[i] << ' ';
            cout << "Dump Finish" << std::endl;
        }

        int getInputSize(){
            int size = 0;
            for(int i = 0;i < argsv.size();++i) size += argsv[i];
            return size;
        }

};