/*
 * query-index.cpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
 *
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../include/util/csv_util.hpp"
#include <chrono>
#include <index/cltj_index_spo_lite.hpp>
#include <iostream>
#include <query/ltj_algorithm.hpp>
#include <triple_pattern.hpp>
#include <util/file_util.hpp>
#include <util/time_util.hpp>
#include <utility>

using namespace std;

// #include<chrono>
// #include<ctime>

using namespace ::util::time;

// Function to print intersection statistics for debugging/experimentation
template <class algorithm_type>
void print_intersection_stats(
    const algorithm_type &ltj,
    uint64_t query_id,
    const string &query_text
) {
  const auto &stats = ltj.get_stats();
  cout << "=== Query " << query_id << " Intersection Statistics ===" << endl;
  cout << "Query text: " << query_text << endl;
  cout << "Total intersections: " << stats.size() << endl;
  for (size_t i = 0; i < stats.size(); ++i) {
    const auto &stat = stats[i];
    cout << "Intersection " << i << ": ";
    cout << "var_id=" << (int)stat.variable_id << ", ";
    cout << "depth=" << stat.depth << ", ";
    cout << "result_size=" << stat.result_size << ", ";
    cout << "min_list_size=" << stat.min_list_size() << ", ";
    cout << "alternation_complexity=" << stat.alternation_complexity << ", ";
    cout << "list_sizes=[";
    for (size_t j = 0; j < stat.list_sizes.size(); ++j) {
      cout << stat.list_sizes[j];
      if (j < stat.list_sizes.size() - 1)
        cout << ",";
    }
    cout << "]" << endl;
  }
  cout << "=============================================" << endl;
}

template <class index_scheme_type, class trait_type>
void query(
    const std::string &file,
    const std::string &queries,
    const uint64_t limit,
    const uint64_t timeout
) {
  vector<string> dummy_queries;
  bool result = ::util::file::get_file_content(queries, dummy_queries);

  index_scheme_type graph;
  sdsl::load_from_file(graph, file);

  std::cout << "Index loaded: " << sdsl::size_in_bytes(graph) << " bytes."
            << std::endl;

  std::ifstream ifs;
  uint64_t nQ = 0;

  //::util::time::usage::usage_type start, stop;
  uint64_t total_elapsed_time;
  uint64_t total_user_time;

  if (result) {
    int count = 1;
    for (string &query_string : dummy_queries) {
      std::vector<ltj::triple_pattern> query =
          ::util::rdf::ids::get_query(query_string);

      typedef ltj::ltj_iterator_lite<index_scheme_type, uint8_t, uint64_t>
          iterator_type;
#if ADAPTIVE
      typedef ltj::ltj_algorithm<
          iterator_type, ltj::veo::veo_adaptive<iterator_type, trait_type>>
          algorithm_type;

#else
      typedef ltj::ltj_algorithm<
          iterator_type, ltj::veo::veo_simple<iterator_type, trait_type>>
          algorithm_type;
#endif
      typedef ::util::results_collector<typename algorithm_type::tuple_type>
          results_type;
      results_type res;

      auto start = std::chrono::high_resolution_clock::now();
      algorithm_type ltj(&query, &graph);
      ltj.join(res, limit, timeout);
      auto stop = std::chrono::high_resolution_clock::now();

      auto time =
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
              .count();
      cout << nQ << ";" << res.size() << ";" << time << endl;

      std::vector<std::string> header = {"query_text",
                                         "var_appearance_order",
                                         "veo_step",
                                         "intersection_size",
                                         "alternation_complexity",
                                         "intersected_list_sizes"};
      util::CSVWriter csv_writer("intersection_statistics.csv", header, 10000);

      static size_t total_rows_written = 0;

      const auto &stats = ltj.get_stats();
      for (size_t i = 0; i < stats.size(); ++i) {
        const auto &stat = stats[i];

        // Build the list sizes string with semicolon separator
        std::string list_sizes_str;
        for (size_t j = 0; j < stat.list_sizes.size(); ++j) {
          list_sizes_str += std::to_string(stat.list_sizes[j]);
          if (j < stat.list_sizes.size() - 1) {
            list_sizes_str += ";";
          }
        }

        // Create csv row with the intersection statistics
        std::vector<std::string> row = {
            query_string,
            std::to_string(stat.variable_id),
            std::to_string(stat.depth),
            std::to_string(stat.result_size),
            std::to_string(stat.alternation_complexity),
            list_sizes_str
        };

        csv_writer.add_row(row);
        total_rows_written++;
      }

      // Write remaining data to the CSV file
      if (nQ == dummy_queries.size() - 1) {
        csv_writer.flush();
        std::cout << "Total rows written: " << total_rows_written << std::endl;
      }

      nQ++;

      count += 1;
    }
  }
}

int main(int argc, char *argv[]) {
  // typedef ring::c_ring ring_type;
  if (argc < 5) {
    std::cout << "Usage: " << argv[0]
              << " <index> <queries> <limit> <type> [timeout]" << std::endl;
    return 0;
  }

  std::string index = argv[1];
  std::string queries = argv[2];
  uint64_t limit = std::atoll(argv[3]);
  std::string type = argv[4];
  uint64_t timeout = 600; // in serconds
  if (argc > 5) {
    timeout = std::atoll(argv[5]);
  }

  if (type == "normal") {
    query<cltj::compact_ltj, ltj::util::trait_distinct>(
        index, queries, limit, timeout
    );
  } else if (type == "star") {
    query<cltj::compact_ltj, ltj::util::trait_size>(
        index, queries, limit, timeout
    );
  } else {
    std::cout << "Type of index: " << type << " is not supported." << std::endl;
  }

  return 0;
}
