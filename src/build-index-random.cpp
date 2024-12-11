#include <iostream>
#include <index/cltj_index_spo_lite.hpp>

using namespace std;

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;

std::random_device rd;
std::mt19937 g(rd());

int main(int argc, char **argv){
    try{

        if(argc != 2){
            cout<< argv[0] << " <dataset>" <<endl;
            return 0;
        }

        std::string dataset = argv[1];
        std::string index_name = dataset + ".random80.cltj";
        vector<cltj::spo_triple> D;

        std::ifstream ifs(dataset);
        uint32_t s, p , o;
        cltj::spo_triple spo;
        do {
            ifs >> s >> p >> o;
            if(ifs.eof()) break;
            spo[0] = s; spo[1] = p; spo[2] = o;
            D.emplace_back(spo);

        } while (!ifs.eof());

        D.shrink_to_fit();
        std::cout << "Dataset: " << 3*D.size()*sizeof(::uint32_t) << " bytes." << std::endl;
        //sdsl::memory_monitor::start();

        std::shuffle(D.begin(), D.end(), g);
        uint64_t n = D.size() * 0.8; //num triples in the build phase
        D.resize(n);

        auto start = timer::now();
        cltj::compact_ltj index(D);
        auto stop = timer::now();

        //sdsl::memory_monitor::stop();

        sdsl::store_to_file(index, index_name);

        cout << "Index saved" << endl;
        cout << duration_cast<seconds>(stop-start).count() << " seconds." << endl;
        cout << sdsl::memory_monitor::peak() << " bytes." << endl;
        // ti.indexNewTable(file_name);

        // ind.save();
    }
    catch(const char *msg){
        cerr<<msg<<endl;
    }
    return 0;
}