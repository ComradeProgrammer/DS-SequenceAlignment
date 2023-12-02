#ifndef __TRACEBACK_TASK_HPP_
#define __TRACEBACK_TASK_HPP_
#include "model/AbstractJsonObject.h"

class TracebackTask : public AbstratJsonObject {
   public:
    virtual std::string toJson() {
        nlohmann::json j;
        j["type"] = "TracebackTask";
        j["x"] = x_;
        j["y"] = y_;
        j["startX"] = start_x_;
        j["startY"] = start_y_;
        return j.dump();
    }
    virtual void loadFromJsonObject(const nlohmann::json& j) {
        x_ = j["x"].template get<int>();
        y_ = j["y"].template get<int>();
        start_x_ = j["startX"].template get<int>();
        start_y_ = j["startY"].template get<int>();
    }

   public:
    // block row number
    int x_;
    // block column number
    int y_;
    // where traceback starts
    int start_x_;
    // where traceback ends
    int start_y_;
};

#endif