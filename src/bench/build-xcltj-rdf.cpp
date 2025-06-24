#include <iostream>
#include <index/cltj_index_spo_dyn.hpp>
#include <dict/dict_map.hpp>
#include <regex>
#include <index/cltj_index_metatrie_dyn.hpp>
#include <util/rdf_util.hpp>

using namespace std;

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;

int main(int argc, char **argv){
    try{

        if(argc != 2){
            cout<< argv[0] << " <dataset>" <<endl;
            return 0;
        }

        std::string dataset = argv[1];
        std::string index_name = dataset + ".xcltj.idx";
        std::string dict_so_name = dataset +  ".xcltj.so";
        std::string dict_p_name = dataset + ".xcltj.p";

        std::ifstream ifs(dataset);
        cltj::spo_triple spo;
        std::string line;

        vector<cltj::spo_triple> D;
        auto start = timer::now();
        std::map<std::string, uint64_t> map_so, map_p;
        std::map<std::string, uint64_t>::iterator it;
        uint64_t id_so = 0, id_p = 0;
        do {
            std::getline(ifs, line);
            if(line.empty()) break;
            auto spo_str = ::util::rdf::str::get_triple(line);
            if((it = map_so.find(spo_str[0])) == map_so.end()) {
                map_so.insert({spo_str[0], ++id_so});
                spo[0] = id_so;
            }else{
                spo[0] = it->second;
            }
            if((it = map_p.find(spo_str[1])) == map_p.end()) {
                map_p.insert({spo_str[1], ++id_p});
                spo[1] = id_p;
            }else{
                spo[1] = it->second;
            }
            if((it=map_so.find(spo_str[2])) == map_so.end()) {
                map_so.insert({spo_str[2], ++id_so});
                spo[2] = id_so;
            }else{
                spo[2] = it->second;
            }
            D.emplace_back(spo);
        } while (!ifs.eof());
        auto stop = timer::now();
        cout << "Mapping: " << duration_cast<seconds>(stop-start).count() << " seconds." << endl;
        D.shrink_to_fit();
        std::cout << "Dataset: " << 3*D.size()*sizeof(::uint32_t) << " bytes." << std::endl;
        //sdsl::memory_monitor::start();
        start = timer::now();
        dict::basic_map dict_so(map_so);
        dict::basic_map dict_p(map_p);
        stop = timer::now();
        cout << "Dictionaries: " << duration_cast<seconds>(stop-start).count() << " seconds." << endl;

        start = timer::now();
        cltj::compact_ltj_metatrie_dyn index(D);
        stop = timer::now();

        //sdsl::memory_monitor::stop();

        sdsl::store_to_file(index, index_name);
        sdsl::store_to_file(dict_p, dict_p_name);
        sdsl::store_to_file(dict_so, dict_so_name);


        std::cout << "Size index : " << sdsl::size_in_bytes(index) << " (bytes)" << std::endl;
        std::cout << "Size dictP : " << sdsl::size_in_bytes(dict_p) << " (bytes)" << std::endl;
        std::cout << "Size dictSO: " << sdsl::size_in_bytes(dict_so) << " (bytes)" << std::endl;

        cout << "Index saved" << endl;
        cout << duration_cast<seconds>(stop-start).count() << " seconds." << endl;
        // ti.indexNewTable(file_name);

        // ind.save();
    }
    catch(const char *msg){
        cerr<<msg<<endl;
    }
    return 0;
}