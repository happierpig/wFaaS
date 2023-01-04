#include <iostream>
#include <vector>

using std::vector;
using std::cin;
using std::cout;

class functionConfiguration{
    public:
        int stack_size, heap_size, return_size;
        int argc;
        vector<int> argsv;

        functionConfiguration() = default;

        ~functionConfiguration() = default;

        void setFromFile(const char* path){

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

        }

        int getInputSize(){
            int size = 0;
            for(int i = 0;i < argsv.size();++i) size += argsv[i];
            return size;
        }

};