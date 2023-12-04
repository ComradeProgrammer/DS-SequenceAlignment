#include "util/sequential_smith_waterman.h"
using namespace std;
using namespace std;
#define SW_HALT 0
#define SW_LEFT 1
#define SW_TOP 2
#define SW_LEFTTOP 3

pair<int,string> sequentialSmithWaterman(
    string row, string column, int match_score, int mismatch_pentalty, int gap,
    vector<vector<pair<int, int>>>& score_matrix ) {
  // calculate the score matrix
  auto get = [](int x, int y,
                const vector<vector<pair<int, int>>>& score_matrix) {
    if (x < 0 || y < 0) {
      return 0;
    }
    return score_matrix[x][y].first;
  };
  int max_score = -1;
  int max_x;
  int max_y;
  for (int i = 0; i < column.size(); i++) {
    score_matrix.push_back(vector<pair<int, int>>(row.size(), {0, 0}));
    for (int j = 0; j < row.size(); j++) {
      int match = column[i] == row[j] ? match_score : mismatch_pentalty;
      int diag = get(i - 1, j - 1, score_matrix) + match;
      int left = get(i, j - 1, score_matrix) + gap;
      int top = get(i - 1, j, score_matrix) + gap;
      int score = max(max(diag, 0), max(left, top));
      score_matrix[i][j].first = score;
      if (score == diag) {
        score_matrix[i][j].second = SW_LEFTTOP;
      } else if (score == left) {
        score_matrix[i][j].second = SW_LEFT;
      } else if (score == top) {
        score_matrix[i][j].second = SW_TOP;
      } else {
        score_matrix[i][j].second = SW_HALT;
      }

      if (score >= max_score) {
        max_score = score;
        max_x = i;
        max_y = j;
      }
    }
  }
  vector<char> res;
  // start traceback
  int x = max_x, y = max_y;
  while (x >= 0 && y >= 0 && score_matrix[x][y].second != SW_HALT) {
    switch (score_matrix[x][y].second) {
      case SW_LEFT:
        res.push_back('-');
        y--;
        break;
      case SW_TOP:
        res.push_back(column[x]);
        x--;
        break;

      case SW_LEFTTOP:
        res.push_back(column[x]);
        x--;
        y--;
        break;
    }
  }
  reverse(res.begin(), res.end());
  string seq;
  seq.insert(seq.begin(), res.begin(), res.end());
  return {max_score,seq};
}
