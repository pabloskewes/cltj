//
// Created by adrian on 29/11/24.
//

#include <dict/dict_map.hpp>
#include <map>
#include <util/rdf_util.hpp>

int main(int argc, char **argv) {

  /*std::map<std::string, uint64_t> map;
  map.insert({"adrian", 1});
  map.insert({"diego", 2});
  map.insert({"fari", 3});
  map.insert({"pedro", 4});

  dict::basic_map map_SO(map);


  std::cout << map_SO.extract(1) << std::endl;
  std::cout << map_SO.extract(2) << std::endl;
  std::cout << map_SO.extract(3) << std::endl;
  std::cout << map_SO.extract(4) << std::endl;

  std::cout << map_SO.locate("adrian") << std::endl;
  std::cout << map_SO.locate("diego") << std::endl;
  std::cout << map_SO.locate("fari") << std::endl;
  std::cout << map_SO.locate("manolo") << std::endl;*/
  std::string dataset = argv[1];
  std::ifstream ifs(dataset);
  std::string line;
  std::map<std::string, uint64_t> map;
  uint64_t id = 0;
  do {
    std::getline(ifs, line);
    if (line.empty())
      break;
    auto spo_str = ::util::rdf::str::get_triple(line);
    if (map.find(spo_str[0]) == map.end()) {
      map.insert({spo_str[0], ++id});
    }
    if (map.find(spo_str[2]) == map.end()) {
      map.insert({spo_str[2], ++id});
    }
  } while (!ifs.eof());

  dict::basic_map map_so(map);
  std::cout << map_so.extract(1) << std::endl;
  std::cout << map_so.extract(2) << std::endl;
  std::cout << map_so.extract(3) << std::endl;
}