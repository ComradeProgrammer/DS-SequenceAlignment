#ifndef __ABSTRACT_JSON_OBJECT_H__
#define __ABSTRACT_JSON_OBJECT_H__
#include <iostream>

#include "nlohmann/json.hpp"

class AbstractJsonObject {
   public:
    virtual std::string toJson() { return toJsonObject().dump(); }
    virtual nlohmann::json toJsonObject() = 0;
    virtual void loadFromJsonObject(const nlohmann::json& j) = 0;

    virtual void loadFromJsonString(const std::string& s) {
        loadFromJsonObject(nlohmann::json::parse(s));
    }
};
#endif