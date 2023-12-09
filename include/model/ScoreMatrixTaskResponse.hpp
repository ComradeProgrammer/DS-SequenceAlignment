#ifndef __SCORE_MATRIX_TASK_RESPONSE_HPP_
#define __SCORE_MATRIX_TASK_RESPONSE_HPP_
#include "model/AbstractJsonObject.h"

class ScoreMatrixTaskResponse : public AbstractJsonObject {
   public:
   virtual ~ScoreMatrixTaskResponse ()=default;
    virtual nlohmann::json toJsonObject() override {
        nlohmann::json j;
        j["type"] = "ScoreMatrixTaskResponse";
        j["bottomRow"] = bottom_row_;
        j["rightColumn"] = right_column_;
        j["x"] = x_;
        j["y"] = y_;
        j["maxScore"] = max_score_;
        j["maxScoreX"] = max_score_x_;
        j["maxScoreY"] = max_score_y_;
        return j;
    }

    virtual void loadFromJsonObject(const nlohmann::json& j) override {
        bottom_row_ = j["bottomRow"].template get<std::vector<int>>();
        right_column_ = j["rightColumn"].template get<std::vector<int>>();
        x_ = j["x"].template get<int>();
        y_ = j["y"].template get<int>();
        max_score_ = j["maxScore"].template get<int>();
        max_score_x_ = j["maxScoreX"].template get<int>();
        max_score_y_ = j["maxScoreY"].template get<int>();
    }

   public:
    std::vector<int> bottom_row_;
    std::vector<int> right_column_;
    int x_;
    int y_;
    int max_score_;
    int max_score_x_;
    int max_score_y_;
};

#endif