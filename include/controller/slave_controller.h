#ifndef __SLAVE_CONTROLLER_H__
#define __SLAVE_CONTROLLER_H__
#include <memory>
#include <string>
#include <thread>

#include "controller/abstract_controller.h"
#include "service/abstract_service.h"
#include "websocketpp/client.hpp"
#include "websocketpp/config/asio_no_tls_client.hpp"
class SlaveController : public AbstractController {
   public:
    SlaveController() = default;
    virtual ~SlaveController() = default;
    /// @brief establish websocket connection with master server or backup
    /// master server, if thet are set
    void establishConnection();

    void onInit();

    void onMasterMessage(
        websocketpp::connection_hdl hdl,
        websocketpp::config::asio_client::message_type::ptr msg);
    void onBackupMasterMessage(
        websocketpp::connection_hdl hdl,
        websocketpp::config::asio_client::message_type::ptr msg);
    void onMasterClose(websocketpp::connection_hdl hdl);
    void onBackupMasterClose(websocketpp::connection_hdl hdl);

    virtual void sendMessageToPeer(const std::string &peer_id,
                                   const std::string &message) override;
    virtual void sendMessageToPeer(const std::string &peer_id, const char *data,
                                   const uint64_t len) override;

    inline void setMasterUri(std::string uri) { master_uri_ = uri; }
    inline void setBackupMasterUri(std::string uri) {
        backup_master_uri_ = uri;
    }
    inline std::shared_ptr<AbstractService> getServiceObject() {
        return service_;
    }
    inline void setID(std::string id) { id_ = id; }

   private:
    std::string id_;
    std::string master_uri_;
    std::string backup_master_uri_;
    websocketpp::client<websocketpp::config::asio_client> master_client_;
    websocketpp::client<websocketpp::config::asio_client>::connection_ptr
        master_connection_;
    websocketpp::client<websocketpp::config::asio_client> backup_master_client_;
    websocketpp::client<websocketpp::config::asio_client>::connection_ptr
        backup_master_connection_;

    std::shared_ptr<AbstractService> service_;
};

#endif