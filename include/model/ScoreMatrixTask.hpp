#ifndef __SCORE_MATRIX_TASK_HPP_
#define __SCORE_MATRIX_TASK_HPP_
#include "model/AbstractJsonObject.h"

class ScoreMatrixTask : public AbstratJsonObject {
   public:
    virtual std::string toJson() {
        nlohmann::json j;
        j["type"] = "ScoreMatrixTask";
        j["leftTopElement"] = left_top_element_;
        j["topRow"] = top_row_;
        j["leftColumn"] = left_column_;
        j["x"] = x_;
        j["y"] = y_;
        j["sequenceRow"] = sequence_row_;
        j["sequenceColumn"] = sequence_column_;
        j["matchScore"] = match_score_;
        j["mismatchPenalty"] = mismatch_pentalty_;
        j["gapExtra"] = gap_extra_;
        j["gapOpen"] = gap_open_;
        return j.dump();
    }
    virtual void loadFromJsonObject(const nlohmann::json& j) {
        left_top_element_ = j["leftTopElement"].template get<int>();
        top_row_ = j["topRow"].template get<std::vector<int>>();
        left_column_ = j["leftColumn"].template get<std::vector<int>>();
        x_ = j["x"].template get<int>();
        y_ = j["y"].template get<int>();
        sequence_row_ = j["sequenceRow"].template get<std::string>();
        sequence_column_ = j["sequenceColumn"].template get<std::string>();
        match_score_ = j["matchScore"].template get<int>();
        mismatch_pentalty_ = j["mismatchPenalty"].template get<int>();
        gap_extra_ = j["gapExtra"].template get<int>();
        gap_open_ = j["gapOpen"].template get<int>();
    }

   public:
    int left_top_element_;
    std::vector<int> left_column_;
    std::vector<int> top_row_;
    int x_;
    int y_;
    std::string sequence_row_;
    std::string sequence_column_;

    int match_score_;
    int mismatch_pentalty_;
    int gap_extra_;
    int gap_open_;
};

#endif