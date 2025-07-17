#include <includelib.hpp>
#include "center.hpp"

int main(int argc, char* argv[]) {  
    if (argc == 1){
        std::cout << "|[Neum:V] "<<argv[0]<<" <filename> vil --gal"<<std::endl;
    }
    else if(argc > 1){
        std::string bedl = std::string (argv[1]);

        if(bedl     == "--gal"){
            std::cout<<help1_server;
        }
        else if(bedl == "--vocem"){
            std::cout<<help2_server;
        }
        else{
            file::read_file<std::string>("hello");
        }

    }
}