#ifndef __SEQUENTIAL_SMITH_WATERMAN_H__
#define __SEQUENTIAL_SMITH_WATERMAN_H__
#include <string>
std::string sequentialSmithWaterman(
    std::string row, std::string column, int match_score,
    int mismatch_pentalty,int gap,
    std::vector<std::vector<std::pair<int, int>>> &score_matrix);
#endif