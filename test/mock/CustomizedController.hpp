#ifndef __CUSTOMIZED_CONTROLLER_H__
#define __CUSTOMIZED_CONTROLLER_H__
#include <functional>

#include "controller/abstract_controller.h"
class CustomizedController : public AbstractController {
   public:
    virtual void sendMessageToPeer(const std::string &peer_id,
                                   const std::string &message) {
        send_message_to_peer_text_(peer_id, message);
    }
    virtual void sendMessageToPeer(const std::string &peer_id, const char *data,
                                   const uint64_t len) {
        send_message_to_peer_binary_(peer_id, data, len);
    }

    std::function<void(const std::string &peer_id, const std::string &message)>
        send_message_to_peer_text_;
    std::function<void(const std::string &peer_id, const char *data,
                       const uint64_t len)>
        send_message_to_peer_binary_;
};
#endif