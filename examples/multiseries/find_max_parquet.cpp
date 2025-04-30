// Copyright 2025 Lawrence Livermore National Security, LLC and other Metall
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#include <iostream>
#include <memory>
#include <filesystem>
#include <string>
#include <cassert>
#include <limits>
#include <typeinfo>
#include <chrono>

#include <arrow/io/file.h>
#include <arrow/util/logging.h>
#include <parquet/api/reader.h>
#include <parquet/api/writer.h>

/// Find the maximum value in a single column chunk of a Parquet file using the
/// parquet reader API.
/// NOTE: this code is hardcoded to read a single column of type int64_t for now.

using parquet::ConvertedType;
using parquet::Repetition;
using parquet::Type;
using parquet::schema::GroupNode;
using parquet::schema::PrimitiveNode;

using value_type = int64_t;

auto start_timer() { return std::chrono::high_resolution_clock::now(); }

auto get_elapsed_time_seconds(
    const std::chrono::time_point<std::chrono::high_resolution_clock>& start) {
  const auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::duration<double>>(end - start)
      .count();
}

// Source:
// https://github.com/apache/arrow/blob/release-16.1.0-rc1/cpp/examples/parquet/low_level_api/reader_writer.cc
value_type read_single_column_chunk(const std::filesystem::path& file_path,
                                    const std::string&           column_name) {
  value_type max_val = std::numeric_limits<value_type>::min();
  try {
    // Create a ParquetReader instance
    std::unique_ptr<parquet::ParquetFileReader> parquet_reader =
        parquet::ParquetFileReader::OpenFile(file_path, false);

    // Get the File MetaData
    std::shared_ptr<parquet::FileMetaData> file_metadata =
        parquet_reader->metadata();

    // Get the number of RowGroups
    const int num_row_groups = file_metadata->num_row_groups();

    // Get the number of Columns
    const int num_columns = file_metadata->num_columns();

    const auto column_index = file_metadata->schema()->ColumnIndex(column_name);

    // Iterate over all the RowGroups in the file
    for (int r = 0; r < num_row_groups; ++r) {
      // Get the RowGroup Reader
      std::shared_ptr<parquet::RowGroupReader> row_group_reader =
          parquet_reader->RowGroup(r);

      int64_t                                values_read = 0;
      int64_t                                rows_read   = 0;
      std::shared_ptr<parquet::ColumnReader> column_reader;

      column_reader = row_group_reader->Column(column_index);
      auto* int64_reader =
          static_cast<parquet::Int64Reader*>(column_reader.get());
      // Read all the rows in the column
      while (int64_reader->HasNext()) {
        value_type value;
        // Read one value at a time. The number of rows read is returned.
        // values_read contains the number of non-null rows
        rows_read =
            int64_reader->ReadBatch(1, nullptr, nullptr, &value, &values_read);

        // Ensure only one value is read
        assert(rows_read == 1);

        // Skip NULL values
        if (values_read == 0) {
          continue;
        }

        max_val = std::max(max_val, value);
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Parquet read error: " << e.what() << std::endl;
  }

  return max_val;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <file_path> <column_name>"
              << std::endl;
    return 1;
  }

  std::filesystem::path file_path{argv[1]};
  std::string           column_name{argv[2]};
  std::cout << "Reading Parquet file: " << file_path << std::endl;
  std::cout << "Value type is: " << typeid(value_type).name() << std::endl;

  try {
    const auto start        = start_timer();
    value_type max_val      = read_single_column_chunk(file_path, column_name);
    const auto elapsed_time = get_elapsed_time_seconds(start);
    std::cout << "Max value in column '" << column_name << "': " << max_val
              << std::endl;
    std::cout << "Elapsed time: " << elapsed_time << " seconds" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
