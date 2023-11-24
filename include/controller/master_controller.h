#ifndef __MASTER_CONTROLLER_H__
#define __MASTER_CONTROLLER_H__
#include <functional>
#include <mutex>
#include <unordered_map>

#include "controller/abstract_controller.h"
#include "crow.h"
class MasterController : AbstractController {
   public:
    MasterController() = default;
    virtual ~MasterController() = default;
    void onInit();
    void onOpen(crow::websocket::connection &conn);
    void onClose(crow::websocket::connection &conn);
    void onMessage(crow::websocket::connection &conn, const std::string &data,
                   bool is_binary);

    virtual void sendMessageToPeer(const std::string &peer_id,
                                   const std::string &message) override;
    virtual void sendMessageToPeer(const std::string &peer_id, const char *data,
                                   const uint64_t len) override;

    void run(uint16_t port);

   private:
    crow::SimpleApp app_;
    std::mutex lock_;
    std::unordered_map<std::string, crow::websocket::connection *>
        id_to_connections;
};

#endif
