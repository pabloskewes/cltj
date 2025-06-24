//
// Created by adrian on 26/11/24.
//

#include <dict/dict_map.hpp>

int main() {

  dict::basic_map map_SO;
  auto id_adrian = map_SO.insert("adrian");
  std::cout << "id_adrian: " << id_adrian << std::endl;
  auto id_pedro = map_SO.insert("pedro");
  std::cout << "id_pedro: " << id_pedro << std::endl;

  id_adrian = map_SO.get_or_insert("adrian");
  auto id_fari = map_SO.get_or_insert("fari");
  std::cout << "id_adrian: " << id_adrian << std::endl;
  std::cout << "id_fari: " << id_fari << std::endl;
  std::cout << map_SO.extract(1) << std::endl;
  std::cout << map_SO.extract(1) << std::endl;
  map_SO.eliminate("adrian");

  auto id_diego = map_SO.insert("diego");
  std::cout << "id_diego: " << id_diego << std::endl;

  std::cout << map_SO.extract(1) << std::endl;
  std::cout << map_SO.extract(2) << std::endl;
  std::cout << map_SO.extract(3) << std::endl;
}