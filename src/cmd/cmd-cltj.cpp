//
// Created by adrian on 3/1/25.
//

#include <string>
#include <api/cltj_ids.hpp>
#include <api/cltj_rdf.hpp>

template<class Index>
void build(const std::string& file,  const std::string &index_name) {
    Index m_index(file);
    sdsl::store_to_file(m_index, index_name);
}


template<class Index>
void interactive(const std::string &index_name) {
    typedef typename Index::tuple_type tuple_type;
    Index m_index;
    if(::util::file::file_exists(index_name)) {
        std::cout << "Loading index from " << index_name << "." << std::endl;
        sdsl::load_from_file(m_index, index_name);
        std::cout << "The index is loaded: " << sdsl::size_in_bytes(m_index) << " bytes." << std::endl;
    }else {
        std::cout << "File " << index_name << " does not exist." << std::endl;
        std::cout << "The index is empty." << std::endl;
    }


    std::cout << "===== CLTJ =====" << std::endl << std::endl;
    uint64_t cnt;
    std::string line, op;
    std::cout << "> " << std::flush;
    std::getline(std::cin, op);
    while(op != "quit") {
        if(op == "commit") {
            std::cout << "! Commit updates... " << std::flush;
            sdsl::store_to_file(m_index, index_name);
            std::cout << "done." << std::endl;
        }else {
            auto in = ::util::rdf::tokenizer(op, ' ');
            op = in[0];
            cnt = std::stoull(in[1]);
            if(op == "insert"){
                std::vector<std::string> to_run;
                for(auto i = 0; i < cnt; ++i) {
                    std::cout << "> [" << i+1 << "/" << cnt << "] " << std::flush;
                    std::getline(std::cin, line);
                    to_run.push_back(line);
                }
                std::cout << "! Running insertions: " << std::flush;
                auto start = std::chrono::high_resolution_clock::now();
                for(auto i = 0; i < cnt; ++i) {
                    m_index.insert(to_run[i]);
                }
                auto stop = std::chrono::high_resolution_clock::now();
                std::cout << cnt << " insertions took " << time << " ns." << std::endl;
                auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            }else if(op == "delete"){
                std::vector<std::string> to_run;
                for(auto i = 0; i < cnt; ++i) {
                    std::cout << "> [" << i+1 << "/" << cnt << "] " << std::flush;
                    std::getline(std::cin, line);
                    to_run.push_back(line);
                }
                std::cout << "! Running deletions: " << std::flush;
                auto start = std::chrono::high_resolution_clock::now();
                for(auto i = 0; i < cnt; ++i) {
                    m_index.remove(to_run[i]);
                }
                auto stop = std::chrono::high_resolution_clock::now();
                std::cout << cnt << " deletions took " << time << " ns." << std::endl;
                auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            }else if (op == "query"){
                std::vector<std::string> to_run;
                for(auto i = 0; i < cnt; ++i) {
                    std::cout << "> [" << i+1 << "/" << cnt << "] " << std::flush;
                    std::getline(std::cin, line);
                    to_run.push_back(line);
                }
                ::util::results_collector<tuple_type> res;
                for(auto i = 0; i < cnt; ++i) {
                    std::cout << "! Running query " << (i+1) << ": " << std::flush;
                    auto start = std::chrono::high_resolution_clock::now();
                    m_index.query(to_run[i], res);
                    auto stop = std::chrono::high_resolution_clock::now();
                    std::cout << res.size() << " results in " << time << " ns." << std::endl;
                    res.clear();
                }
            }else{
                std::cout << "! Operation " << op << " is not supported." << std::endl;
            }
        }
        std::cout << "> " << std::flush;
        std::getline(std::cin, op);
    }
}

struct build_args {
    std::string file;
    std::string index;
    std::string tries = "partial";
};

struct run_args {
    std::string index;
    std::string veo = "adaptive";
    std::string tries = "partial";
};

//function to parse the command line arguments
//this function will return a struct with the parsed arguments
//the parse string is the following:
//  <exec> <file> <index> [tries]
build_args parse_build_args(int argc, char **argv) {
    build_args args;
    args.file = argv[2];
    args.index = argv[3];
    if(argc > 4) {
        args.tries = argv[4];
    }
    return args;
}

//function to parse the command line arguments
//this function will return a struct with the parsed arguments
//the parse string is the following:
// <exec> <index> [veo]
run_args parse_run_args(int argc, char **argv) {
    run_args args;
    args.index = argv[2];
    if(argc > 3) {
        args.veo = argv[3];
    }
    return args;
}

int main(int argc, char **argv) {

    std::string exec = argv[1];
    if(exec == "build") {
        if(argc < 4) {
            std::cout << "Usage: " << argv[0] << " build <file> <index> [tries]" << std::endl;
            return 0;
        }
        auto args = parse_build_args(argc, argv);
        if(args.tries == "partial") {
            build<cltj::cltj_mt_ids_dyn>(args.file, args.index+ ".xcltj");
        }else if(args.tries == "full") {
            build<cltj::cltj_ids_dyn>(args.file, args.index + ".cltj");
        }else{
            std::cout << "Tries " << args.tries << " is not supported." << std::endl;
        }
    }else if (exec == "run") {
        if(argc < 3) {
            std::cout << "Usage: " << argv[0] << " run <index> [veo] [update]" << std::endl;
            return 0;
        }
        auto args = parse_run_args(argc, argv);
        auto ext = ::util::file::get_extension(args.index);
        if(ext == ".xcltj") {
            if(args.veo == "adaptive") {
                interactive<cltj::cltj_mt_ids_dyn>(args.index);
            }else if(args.veo == "simple") {
                interactive<cltj::cltj_mt_ids_dyn_global>(args.index);
            }else {
                std::cout << "Veo " << args.veo << " is not supported." << std::endl;
            }
        }else if (ext == ".cltj") {
            if(args.veo == "adaptive") {
                interactive<cltj::cltj_ids_dyn>(args.index);
            }else if(args.veo == "simple") {
                interactive<cltj::cltj_ids_dyn_global>(args.index);
            }else {
                std::cout << "Veo " << args.veo << " is not supported." << std::endl;
            }
        }else {
            std::cout << "Extension " << ext << " is not supported." << std::endl;
        }
    }
}
