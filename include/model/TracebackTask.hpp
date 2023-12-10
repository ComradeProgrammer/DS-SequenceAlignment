#ifndef __TRACEBACK_TASK_HPP_
#define __TRACEBACK_TASK_HPP_
#include "model/AbstractJsonObject.h"
#include "model/AbstractTask.hpp"

class TracebackTask : public AbstractTask {
   public:
   virtual ~TracebackTask()=default;

    virtual nlohmann::json toJsonObject() override{
        nlohmann::json j;
        j["type"] = "TracebackTask";
        j["x"] = x_;
        j["y"] = y_;
        j["startX"] = start_x_;
        j["startY"] = start_y_;
        j["rowID"] = row_id_;
        return j;
    }
    virtual void loadFromJsonObject(const nlohmann::json& j)override {
        x_ = j["x"].template get<int>();
        y_ = j["y"].template get<int>();
        start_x_ = j["startX"].template get<int>();
        start_y_ = j["startY"].template get<int>();
        row_id_ = j["rowID"].template get<int>();

    }

    virtual std::string getShortName() override {
        return "TracebackTask["+std::to_string(row_id_)+"](" + std::to_string(x_) + "," + std::to_string(y_) + ")";
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

    int row_id_;
    
};

#endif