#ifndef __MASTER_CONTROLLER_H__
#define __MASTER_CONTROLLER_H__
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "config/configuration.h"
#include "controller/abstract_controller.h"
#include "crow.h"
#include "service/abstract_service.h"

class MasterController : public AbstractController {
   public:
    MasterController() = default;
    virtual ~MasterController() = default;
    void onInit(std::shared_ptr<Configuration> config, bool backup);
    void onOpen(crow::websocket::connection &conn);
    void onClose(crow::websocket::connection &conn);
    void onMessage(crow::websocket::connection &conn, const std::string &data,
                   bool is_binary);

    virtual void sendMessageToPeer(const std::string &peer_id,
                                   const std::string &message) override;
    virtual void sendMessageToPeer(const std::string &peer_id, const char *data,
                                   const uint64_t len) override;

    void run(uint16_t port);
    inline std::shared_ptr<AbstractService> getServiceObject() {
        return service_;
    }

   private:
    crow::SimpleApp app_;
    std::mutex lock_;
    std::unordered_map<std::string, crow::websocket::connection *>
        id_to_connections;
    std::shared_ptr<AbstractService> service_;
};

#endif
