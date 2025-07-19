#ifndef CSV_UTIL_HPP
#define CSV_UTIL_HPP

#include <fstream>
#include <string>
#include <vector>

namespace util {

/**
 * @brief A class to write data to a CSV file
 * @param filename The name of the file to write to
 * @param header The header of the CSV file
 * @param batch_size The number of rows to buffer before writing to the file
 */
class CSVWriter {
public:
  CSVWriter(
      const std::string &filename,
      const std::vector<std::string> &header,
      size_t batch_size = 10000
  )
      : filename_(filename), batch_size_(batch_size) {
    std::ofstream file(filename_, std::ios::app);
    if (!file.is_open()) {
      throw std::runtime_error("Could not open file for writing");
    }
    // Write header only if the file is new
    if (file.tellp() == 0) {
      write_row(file, header);
    }
  }

  ~CSVWriter() {
    flush();
  }

  void add_row(const std::vector<std::string> &row) {
    buffer_.push_back(row);
    if (buffer_.size() >= batch_size_) {
      flush();
    }
  }

  void flush() {
    std::ofstream file(filename_, std::ios::app);
    if (!file.is_open()) {
      throw std::runtime_error("Could not open file for writing");
    }
    for (const auto &row : buffer_) {
      write_row(file, row);
    }
    buffer_.clear();
  }

private:
  std::string filename_;
  size_t batch_size_;
  std::vector<std::vector<std::string>> buffer_;

  void write_row(std::ofstream &file, const std::vector<std::string> &row) {
    for (size_t i = 0; i < row.size(); ++i) {
      file << row[i];
      if (i < row.size() - 1) {
        file << ",";
      }
    }
    file << "\n";
  }
};

} // namespace util

#endif // CSV_UTIL_HPP