#include "util/util.h"
using namespace std;
string charArrayToString(const char* data, const uint64_t len) {
    stringstream ss;
    for (int i = 0; i < len; i++) {
        ss << int(data[i]) << " ";
    }
    return ss.str();
}
