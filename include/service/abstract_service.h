#ifndef __ABSTRACT_SERVICE_H__
#define __ABSTRACT_SERVICE_H__
#include <cstdlib>
#include <string>
class AbstractController;
class AbstractService {
   public:
    AbstractService(AbstractController* controller) : controller_(controller){};
    virtual void onInit()=0;
    virtual void onNewMessage(std::string peer_id, std::string&& message,
                              bool is_binary) = 0;
    virtual void onConnectionEstablished(const std::string peer_id) = 0;
    virtual void onConnectionTerminated(const std::string peer_id) = 0;
    virtual void sendMessageToPeer(const std::string& peer_id,
                                   const std::string& message);
    virtual void sendMessageToPeer(const std::string& peer_id, const char* data,
                                   const uint64_t len) ;

   protected:
    AbstractController* controller_;
};

#endif