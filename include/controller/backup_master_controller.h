#ifndef __BACKUP_MASTER_CONTROLLER_H__
#define __BACKUP_MASTER_CONTROLLER_H__
#include <memory>
#include <string>
#include <thread>

#include "controller/abstract_controller.h"
#include "crow.h"
#include "service/abstract_service.h"
#include "websocketpp/client.hpp"
#include "websocketpp/config/asio_no_tls_client.hpp"
class BackupMasterController : public AbstractController {
   public:
    void establishConnection();
    void onInit();
    void run(uint16_t port);
    void onOpen(crow::websocket::connection &conn);
    void onClose(crow::websocket::connection &conn);
    void onMessage(crow::websocket::connection &conn, const std::string &data,
                   bool is_binary);
    void onMasterMessage(
        websocketpp::connection_hdl hdl,
        websocketpp::config::asio_client::message_type::ptr msg);
    
    void onMasterClose(websocketpp::connection_hdl hdl);
    virtual void sendMessageToPeer(const std::string &peer_id,
                                   const std::string &message) override;
    virtual void sendMessageToPeer(const std::string &peer_id, const char *data,
                                   const uint64_t len) override;
    inline void setMasterUri(std::string uri) { master_uri_ = uri; }
    inline std::shared_ptr<AbstractService> getServiceObject() {
        return service_;
    }

   private:
    std::mutex lock_;
    std::unordered_map<std::string, crow::websocket::connection *>
        id_to_connections;
    crow::SimpleApp app_;
    std::string master_uri_;
    websocketpp::client<websocketpp::config::asio_client> master_client_;
    websocketpp::client<websocketpp::config::asio_client>::connection_ptr
        master_connection_;
    std::shared_ptr<AbstractService> service_;
};

#endif