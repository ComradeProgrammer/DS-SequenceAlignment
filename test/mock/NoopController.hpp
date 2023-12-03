#ifndef __NOOP_CONTROLLER_H__
#define __NOOP_CONTROLLER_H__

#include "controller/abstract_controller.h"
class NoopController : public AbstractController {
   public:
    virtual void sendMessageToPeer(const std::string &peer_id,
                                   const std::string &message) {
        // do nothing
    }
    virtual void sendMessageToPeer(const std::string &peer_id, const char *data,
                                   const uint64_t len) {
        // do nothing
    }
};
#endif