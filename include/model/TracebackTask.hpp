#ifndef __TRACEBACK_TASK_HPP_
#define __TRACEBACK_TASK_HPP_
#include "model/AbstractJsonObject.h"

class TracebackTask : public AbstratJsonObject {
   public:
    virtual std::string toJson(){

    }
    virtual void loadFromJsonObject(const nlohmann::json& j) {

    }

    public:

    
};

#endif