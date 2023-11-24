#ifndef __WS_CONTROLLER_H__
#define __WS_CONTROLLER_H__


#include <string>

#define MASTER_ID "#master"
#define BACKUP_MASTER_ID "#backup_master"

class AbstractController {
   public:
    virtual void sendMessageToPeer(const std::string &peer_id,
                                   const std::string &message) = 0;
    virtual void sendMessageToPeer(const std::string &peer_id, const char *data,
                                   const uint64_t len) = 0;

};
#endif