#ifndef CSV_UTIL_HPP
#define CSV_UTIL_HPP

#include <fstream>
#include <string>
#include <vector>

namespace util {

/**
 * Write data to a CSV file with an optional header
 * @param filename The name of the file to write to
 * @param header The header of the CSV file
 * @param data The data to write to the CSV file
 */
void write_to_csv(
    const std::string &filename,
    const std::vector<std::string> &header,
    const std::vector<std::vector<std::string>> &data
) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file for writing");
  }

  if (data.empty()) {
    throw std::runtime_error("Data is empty");
  }

  if (!header.empty()) {
    if (header.size() != data[0].size()) {
      throw std::runtime_error("Header and data size mismatch");
    }

    for (size_t i = 0; i < header.size(); ++i) {
      file << header[i];
      if (i < header.size() - 1) {
        file << ",";
      }
    }
    file << "\n";
  }

  for (const auto &row : data) {
    for (size_t i = 0; i < row.size(); ++i) {
      file << row[i];
      if (i < row.size() - 1) {
        file << ",";
      }
    }
    file << "\n";
  }

  file.close();
}

} // namespace util

#endif // CSV_UTIL_HPP