#ifndef __MASTER_SERVICE_H__
#define __MASTER_SERVICE_H__
#include "service/abstract_service.h"
class MasterService : public AbstractService {
    public:
    MasterService(AbstractController* controller);
    virtual void onInit() override;
    virtual void onNewMessage(std::string peer_id, std::string&& message,
                              bool is_binary)override;
    virtual void onConnectionEstablished(const std::string peer_id)override;
    virtual void onConnectionTerminated(const std::string peer_id)override;
};

#endif