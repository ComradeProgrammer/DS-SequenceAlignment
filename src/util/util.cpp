#include "util/util.h"

#include <fstream>

#include "crow/logging.h"
using namespace std;
string charArrayToString(const char* data, const uint64_t len) {
    stringstream ss;
    for (int i = 0; i < len; i++) {
        ss << int(data[i]) << " ";
    }
    return ss.str();
}
void readFastaFile(string file_name, string& out_res) {
    stringstream ss;
    ifstream f(file_name, ios::in);
    if (!f.is_open()) {
        CROW_LOG_ERROR << "failed to open the file " << file_name;
        return;
    }
    string line;
    while (getline(f, line)) {
        if (line.empty() || line[0] == '>') continue;
        line.erase(0, line.find_first_not_of(" \r\n\t\v\f"));
        line.erase(line.find_last_not_of(" \r\n\t\v\f") + 1);
        ss << line;
    }
    out_res = ss.str();
}
