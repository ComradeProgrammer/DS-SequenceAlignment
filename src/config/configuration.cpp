#include "config/configuration.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "crow/logging.h"
#include "nlohmann/json.hpp"

using namespace std;
using nlohmann::json;
bool Configuration::loadFromFile(string file_path) {
    ifstream config_file;
    config_file.open(file_path, ios::in);
    if (!config_file.is_open()) {
        CROW_LOG_ERROR << "failed to read file: " << file_path;
        return false;
    }

    json j;
    try {
        j = json::parse(config_file);
    } catch (const json::exception& e) {
        CROW_LOG_ERROR << "failed to parse: " << e.what();
        return false;
    }

    if (j.contains("masterWaitTime")) {
        master_wait_time = j["masterWaitTime"].template get<int>();
    }
    if (j.contains("columnBlockSize")) {
        column_block_size_ = j["columnBlockSize"].template get<int>();
    }
    if (j.contains("rowBlockSize")) {
        row_block_size_ = j["rowBlockSize"].template get<int>();
    }

    if (j.contains("sequenceRowType")) {
        sequence_row_type_ = j["sequenceRowType"].template get<string>();
    }

    if (j.contains("sequenceRowDataSource")) {
        sequence_row_data_source_ =
            j["sequenceRowDataSource"].template get<string>();
    }

    if (j.contains("sequenceColumnType")) {
        sequence_column_type_ = j["sequenceRowType"].template get<string>();
    }

    if (j.contains("sequenceColumnDataSource")) {
        sequence_column_data_source_ =
            j["sequenceColumnDataSource"].template get<string>();
    }

    if (j.contains("matchScore")) {
        match_score_ = j["matchScore"].template get<int>();
    }
    if (j.contains("matchScore")) {
        match_score_ = j["matchScore"].template get<int>();
    }
    if (j.contains("mismatchPenalty")) {
        mismatch_penalty_ = j["mismatchPenalty"].template get<int>();
    }
    if (j.contains("gapOpen")) {
        gap_open_ = j["gapOpen"].template get<int>();
    }
    return true;
}