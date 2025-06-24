//
// Created by adrian on 3/1/25.
//

#include <api/cltj_ids.hpp>
#include <string>

const std::string RESET = "\033[0m";
const std::string RED = "\033[1;31m";
const std::string GREEN = "\033[1;32m";
const std::string BLUE = "\033[1;36m";

template <class Index>
void build(const std::string &file, const std::string &index_name) {
  Index m_index(file);
  sdsl::store_to_file(m_index, index_name);
}

void print_logo() {
  std::cout << GREEN << R"(
   ______  _____   _________   _____
 .' ___  ||_   _| |  _   _  | |_   _|
/ .'   \_|  | |   |_/ | | \_|   | |
| |         | |   _   | |   _   | |
\ `.___.'\ _| |__/ | _| |_ | |__' |
 `.____ .'|________||_____|`.____.'

              )"
            << RESET << "\n\n";
}

template <class Index>
void interactive(
    const std::string &index_name,
    uint64_t limit,
    uint64_t timeout,
    bool print
) {
  typedef typename Index::tuple_type tuple_type;
  Index m_index;
  if (::util::file::file_exists(index_name)) {
    std::cout << BLUE << "Loading index from " << index_name << "."
              << std::endl;
    sdsl::load_from_file(m_index, index_name);
    std::cout << "The index is loaded: " << sdsl::size_in_bytes(m_index)
              << " bytes." << RESET << std::endl
              << std::endl;
  } else {
    std::cout << BLUE << "File " << index_name << " does not exist."
              << std::endl;
    std::cout << "The index is empty." << RESET << std::endl << std::endl;
  }

  uint64_t cnt;
  std::string line, op;
  std::cout << GREEN << "[CLTJ]> " << RESET << std::flush;
  std::getline(std::cin, op);
  while (op != "quit") {
    if (op == "commit") {
      std::cout << "       " << RED << " Commit updates... " << std::flush;
      sdsl::store_to_file(m_index, index_name);
      std::cout << "done." << RESET << std::endl;
    } else {
      auto in = ::util::rdf::tokenizer(op, ' ');
      if (in.size() < 2) {
        std::cout << "    ! Operation " << op << " is not supported."
                  << std::endl;
      } else {
        op = in[0];
        cnt = std::stoull(in[1]);
        if (op == "insert") {
          std::vector<std::string> to_run;
          for (auto i = 0; i < cnt; ++i) {
            std::cout << GREEN << "[CLTJ]> " << BLUE << "[" << i + 1 << "/"
                      << cnt << "] " << RESET << std::flush;
            std::getline(std::cin, line);
            to_run.push_back(line);
          }
          std::cout << "        " << RED << "[" << cnt << "/" << cnt << "] "
                    << RESET << std::flush;
          uint64_t ins = 0;
          auto start = std::chrono::high_resolution_clock::now();
          for (auto i = 0; i < cnt; ++i) {
            ins += m_index.insert(to_run[i]);
          }
          auto stop = std::chrono::high_resolution_clock::now();
          auto time =
              std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
                  .count();
          std::cout << ins << " triples inserted in " << time << " ns."
                    << std::endl;
        } else if (op == "delete") {
          std::vector<std::string> to_run;
          for (auto i = 0; i < cnt; ++i) {
            std::cout << GREEN << "[CLTJ]> " << BLUE << "[" << i + 1 << "/"
                      << cnt << "] " << RESET << std::flush;
            std::getline(std::cin, line);
            to_run.push_back(line);
          }
          uint64_t dels = 0;
          std::cout << "        " << RED << "[" << cnt << "/" << cnt << "] "
                    << RESET << std::flush;
          auto start = std::chrono::high_resolution_clock::now();
          for (auto i = 0; i < cnt; ++i) {
            dels += m_index.remove(to_run[i]);
          }
          auto stop = std::chrono::high_resolution_clock::now();
          auto time =
              std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
                  .count();
          std::cout << dels << " triples deleted in " << time << " ns."
                    << std::endl;
        } else if (op == "query") {
          std::vector<std::string> to_run;
          for (auto i = 0; i < cnt; ++i) {
            std::cout << GREEN << "[CLTJ]> " << BLUE << "[" << i + 1 << "/"
                      << cnt << "] " << RESET << std::flush;
            std::getline(std::cin, line);
            to_run.push_back(line);
          }
          if (print) {
            ::util::results_printer<uint64_t> res;
            for (auto i = 0; i < cnt; ++i) {
              std::cout << "        " << RED << "[" << i + 1 << "/" << cnt
                        << "] begin" << RESET << std::endl;
              auto start = std::chrono::high_resolution_clock::now();
              m_index.query(to_run[i], res, limit, timeout);
              auto stop = std::chrono::high_resolution_clock::now();
              auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              stop - start
              )
                              .count();
              std::cout << "        " << RED << "[" << i + 1 << "/" << cnt
                        << "] end" << RESET << std::endl;
              std::cout << "        " << RED << "[" << i + 1 << "/" << cnt
                        << "] " << RESET << res.size() << " results in " << time
                        << " ns." << std::endl;
              res.clear();
            }
          } else {
            ::util::results_collector<tuple_type> res;
            for (auto i = 0; i < cnt; ++i) {
              std::cout << "        " << RED << "[" << i + 1 << "/" << cnt
                        << "] " << RESET << std::flush;
              auto start = std::chrono::high_resolution_clock::now();
              m_index.query(to_run[i], res, limit, timeout);
              auto stop = std::chrono::high_resolution_clock::now();
              auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              stop - start
              )
                              .count();
              std::cout << res.size() << " results in " << time << " ns."
                        << std::endl;
              res.clear();
            }
          }

        } else {
          std::cout << "    ! Operation " << op << " is not supported."
                    << std::endl;
        }
      }
    }
    std::cout << GREEN << "[CLTJ]> " << RESET << std::flush;
    std::getline(std::cin, op);
  }
}

struct build_args {
  std::string file;
  std::string index;
  std::string tries = "partial";
};

// print build_args
void print(build_args &ba) {
  std::cout << BLUE << "CLTJ IDs Version" << std::endl;
  std::cout << GREEN << "file= " << ba.file << std::endl;
  std::cout << "index= " << ba.index << std::endl;
  std::cout << "tries= " << ba.tries << RESET << std::endl << std::endl;
}

struct run_args {
  std::string index;
  std::string veo = "adaptive";
  bool print = false;
  uint64_t limit = 1000;
  uint64_t timeout = 600;
};

void print(run_args &ra) {
  std::cout << BLUE << "CLTJ IDs Version" << std::endl;
  std::cout << GREEN << "index= " << ra.index << std::endl;
  std::cout << "print=" << ra.print << std::endl;
  std::cout << "limit=" << ra.limit << std::endl;
  std::cout << "timeout=" << ra.timeout << std::endl;
  std::cout << "veo=" << ra.veo << RESET << std::endl << std::endl;
}

// function to parse the command line arguments
// this function will return a struct with the parsed arguments
// the parse string is the following:
//   <exec> <file> <index> [tries]
build_args parse_build_args(int argc, char **argv) {
  build_args args;
  args.file = argv[2];
  args.index = argv[3];
  if (argc > 4) {
    args.tries = argv[4];
  }
  return args;
}

// function to parse the command line arguments
// this function will return a struct with the parsed arguments
// the parse string is the following:
//  <exec> <index> [print] [limit] [timeout] [veo]
run_args parse_run_args(int argc, char **argv) {
  run_args args;
  args.index = argv[2];
  if (argc > 3) {
    args.print = std::string(argv[3]) == "print";
  }
  if (argc > 4) {
    args.limit = std::stoull(argv[4]);
  }
  if (argc > 5) {
    args.timeout = std::stoull(argv[5]);
  }
  if (argc > 6) {
    args.veo = argv[6];
  }
  return args;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <exec> [args]" << std::endl;
    std::cout << "Exec: build <file> <index> [version]" << std::endl;
    std::cout << "  <file>: the dataset file." << std::endl;
    std::cout << "  <index>: the index file." << std::endl;
    std::cout << "  <version>: the kind of version to use: xcltj or cltj. "
                 "Default is xcltj."
              << std::endl;
    std::cout << "Exec: run <index> [veo] [print] [limit] [timeout]"
              << std::endl;
    std::cout << "  <index>: the index file." << std::endl;
    std::cout << "  <print>: the kind of print to use: none or print. Default "
                 "is none."
              << std::endl;
    std::cout << "  <limit>: the limit of results. Default is 1000. Setting to "
                 "0 means no limit."
              << std::endl;
    std::cout
        << "  <timeout>: the timeout of the query in seconds. Default is 600."
        << std::endl;
    std::cout << "  <veo>: the kind of veo to use: adaptive or global. Default "
                 "is adaptive."
              << std::endl;
    return 0;
  }
  std::string exec = argv[1];
  if (exec == "build") {
    if (argc < 4) {
      std::cout << "Usage: " << argv[0] << " build <file> <index> [version]"
                << std::endl;
      std::cout << "  <file>: the dataset file." << std::endl;
      std::cout << "  <index>: the index file." << std::endl;
      std::cout << "  <version>: the kind of version to use: xcltj or cltj. "
                   "Default is xcltj."
                << std::endl;
      return 0;
    }
    auto args = parse_build_args(argc, argv);
    if (args.tries == "partial") {
      print_logo();
      print(args);
      build<cltj::xcltj_ids_dyn>(args.file, args.index + ".xcltj");
    } else if (args.tries == "full") {
      print_logo();
      print(args);
      build<cltj::cltj_ids_dyn>(args.file, args.index + ".cltj");
    } else {
      std::cout << "Tries " << args.tries << " is not supported." << std::endl;
    }
  } else if (exec == "run") {
    if (argc < 3) {
      std::cout << "Usage: " << argv[0]
                << " run <index> [print] [limit] [timeout] [veo]" << std::endl;
      std::cout << "  <index>: the index file." << std::endl;
      std::cout << "  <print>: the kind of print to use: none or print. "
                   "Default is none."
                << std::endl;
      std::cout << "  <limit>: the limit of results. Default is 1000. Setting "
                   "to 0 means no limit."
                << std::endl;
      std::cout
          << "  <timeout>: the timeout of the query in seconds. Default is 600."
          << std::endl;
      std::cout << "  <veo>: the kind of veo to use: adaptive or global. "
                   "Default is adaptive."
                << std::endl;
      return 0;
    }
    auto args = parse_run_args(argc, argv);
    auto ext = ::util::file::get_extension(args.index);
    if (ext == ".xcltj") {
      print_logo();
      print(args);
      if (args.veo == "adaptive") {
        interactive<cltj::xcltj_ids_dyn>(
            args.index, args.limit, args.timeout, args.print
        );
      } else if (args.veo == "global") {
        interactive<cltj::xcltj_ids_dyn_global>(
            args.index, args.limit, args.timeout, args.print
        );
      } else {
        std::cout << "Veo " << args.veo << " is not supported." << std::endl;
      }
    } else if (ext == ".cltj") {
      print_logo();
      print(args);
      if (args.veo == "adaptive") {
        interactive<cltj::cltj_ids_dyn>(
            args.index, args.limit, args.timeout, args.print
        );
      } else if (args.veo == "global") {
        interactive<cltj::cltj_ids_dyn_global>(
            args.index, args.limit, args.timeout, args.print
        );
      } else {
        std::cout << "Veo " << args.veo << " is not supported." << std::endl;
      }
    } else {
      std::cout << "Extension " << ext << " is not supported." << std::endl;
    }
  }
}
