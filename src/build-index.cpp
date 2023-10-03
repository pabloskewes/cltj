#include <iostream>
#include <cltj_table_indexing.hpp>

using namespace std;

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;

int main(int argc, char **argv){
    try{

        if(argc <= 1){
            cout<<"No extra command line argument given other that program name"<<endl;
            return 0;
        }


        cltj::TableIndexer ti = cltj::TableIndexer();


        // for(int i=1; i<argc; i++){

        // }

        string file_name = argv[1];
        ti.readTable(file_name);
        memory_monitor::start();
        auto start = timer::now();

        ti.indexNewTable(file_name);
        auto stop = timer::now();
        memory_monitor::stop();

        ti.saveIndex();

        cout << "Index saved" << endl;
        cout << duration_cast<seconds>(stop-start).count() << " seconds." << endl;
        cout << memory_monitor::peak() << " bytes." << endl;
        // ti.indexNewTable(file_name);

        // ind.save();
    }
    catch(const char *msg){
        cerr<<msg<<endl;
    }
    return 0;
}