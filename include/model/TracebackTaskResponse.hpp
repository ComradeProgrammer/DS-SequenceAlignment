#ifndef __TRACEBACK_TASK_RESPONSE_HPP_
#define __TRACEBACK_TASK_RESPONSE_HPP_
#include "model/AbstractJsonObject.h"

class TracebackTaskResponse : public AbstratJsonObject {
   public:
    virtual std::string toJson(){

    }
    virtual void loadFromJsonObject(const nlohmann::json& j) {

    }

    public:

    
};

#endif