#ifndef __TRACEBACK_TASK_RESPONSE_HPP_
#define __TRACEBACK_TASK_RESPONSE_HPP_
#include "model/AbstractJsonObject.h"

class TracebackTaskResponse : public AbstratJsonObject {
   public:
    virtual std::string toJson() {
        nlohmann::json j;
        j["type"] = "TracebackTaskResponse";
        j["x"] = x_;
        j["y"] = y_;
        j["endX"] = end_x_;
        j["endY"] = end_y_;
        j["sequence"] = sequence_;
        return j.dump();
    }
    virtual void loadFromJsonObject(const nlohmann::json& j) {
        x_ = j["x"].template get<int>();
        y_ = j["y"].template get<int>();
        end_x_ = j["endX"].template get<int>();
        end_y_ = j["endY"].template get<int>();
        sequence_ = j["sequence"].template get<std::string>();
    }

   public:
    // returned sequence
    std::string sequence_;
    // block row number
    int x_;
    // block column number
    int y_;
    // location where the traceback ends
    int end_x_;
    // location where the traceback ends
    int end_y_;
};

#endif