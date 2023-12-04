#ifndef __SCORE_MATRIX_BLOCK_H__
#define __SCORE_MATRIX_BLOCK_H__
#include <memory>
#include <vector>

#include "model/ScoreMatrixTask.hpp"
#define SW_HALT 0
#define SW_LEFT 1
#define SW_TOP 2
#define SW_LEFTTOP 3
class ScoreMatrixBlock {
   public:
    int max_score_=-1;
    int max_score_x_;
    int max_score_y_;
    std::vector<std::vector<std::pair<int,int>>>score_matrix_;
    std::shared_ptr<ScoreMatrixTask> task_;
};

#endif