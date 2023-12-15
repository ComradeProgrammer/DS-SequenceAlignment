#include <sys/time.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include "config/configuration.h"
#include "util/util.h"
#include "util/sequential_smith_waterman.h"
using namespace std;
void getSequence(string data_type, string data_source, string& out_res) {
    
    if (data_type == "string") {
        out_res = data_source;
    } else if (data_type == "text") {
        // read the text string
        stringstream ss;
        ifstream f(data_source, ios::in);
        ss << f.rdbuf();
        out_res = ss.str();
        // remove the space
        out_res.erase(0, out_res.find_first_not_of(" \r\n\t\v\f"));
        out_res.erase(out_res.find_last_not_of(" \r\n\t\v\f") + 1);
    }
}
int getTimestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main() {
    auto config = std::make_shared<Configuration>();
    config->loadFromFile("config.json");
    string a, b;
    getSequence(config->sequence_column_type_,
                config->sequence_column_data_source_, a);
    getSequence(config->row_sequences[0].sequence_row_type_,
                config->row_sequences[0].sequence_row_data_source_, b);
    vector<vector<pair<int, int>>> tmp;
    long long t1 = getTimestamp();
    sequentialSmithWaterman(a, b, config->match_score_,
                            config->mismatch_penalty_, config->gap_open_, tmp);
    cout << getTimestamp() - t1 << endl;
}