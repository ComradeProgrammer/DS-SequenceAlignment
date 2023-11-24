#include "controller/master_controller.h"

#include "model/IdentificationInfo.hpp"
#include "nlohmann/json.hpp"
#include "util/util.h"

using nlohmann::json;
using std::bind;
using std::lock_guard;
using std::string;
using std::mutex;

void MasterController::onInit() {
    // create a websocket server
    CROW_WEBSOCKET_ROUTE(app_, "/websocket")
        .onopen(bind(&MasterController::onOpen, this, std::placeholders::_1))
        .onclose(bind(&MasterController::onClose, this, std::placeholders::_1))
        .onmessage(bind(&MasterController::onMessage, this,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3));
}

void MasterController::onOpen(crow::websocket::connection& conn) {
    CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
    // do nothing, we need to wait for IdentificationInfo packet
}

void MasterController::onClose(crow::websocket::connection& conn) {
    // check whether this connection has IdentificationInfo attached
    if (conn.userdata() == nullptr) {
        CROW_LOG_INFO << "new websocket connection from "
                      << conn.get_remote_ip();
    } else {
        auto tmp = static_cast<IdentificationInfo*>(conn.userdata());
        conn.userdata(nullptr);
        CROW_LOG_INFO << "new websocket connection from " << tmp->getPeerID()
                      << " " << conn.get_remote_ip();

        lock_.lock();
        id_to_connections.erase(tmp->getPeerID());
        lock_.unlock();
        delete tmp;
    }
}

void MasterController::onMessage(crow::websocket::connection& conn,
                                 const std::string& data, bool is_binary) {
    // first we need to check whether it is an IdentificationInfo json object
    if (!is_binary) {
        CROW_LOG_INFO << "new text message " << data;
        json j;
        bool parse_ok = true;
        try {
            j = json::parse(data);
        } catch (const json::exception& e) {
            parse_ok = false;
        }
        if (parse_ok && j.contains("type") &&
            j["type"].template get<string>() == "IdentificationInfo") {
            lock_guard<mutex>block_lock(lock_);
            // store client's info in the connection's user data
            IdentificationInfo* user_info = new IdentificationInfo();
            user_info->loadFromJsonObject(j);
            conn.userdata(user_info);
            // and store the connection into the map
            string user_id = user_info->is_backup_master_
                                 ? BACKUP_MASTER_ID
                                 : user_info->getPeerID();
            id_to_connections[user_id] = &conn;
            return;
        }
    } else {
        CROW_LOG_INFO << "new data message "
                      << charArrayToString(data.c_str(), data.size());
    }
    // forward this message to the service layer
}

void MasterController::sendMessageToPeer(const std::string& peer_id,
                                         const std::string& message) {
    lock_.lock();
    // check whether this peer_id exists
    if (id_to_connections.find(peer_id) == id_to_connections.end()) {
        CROW_LOG_WARNING << peer_id << " does not exist";
    }
    auto connection = id_to_connections[peer_id];
    lock_.unlock();
    connection->send_text(message);
}

void MasterController::sendMessageToPeer(const std::string& peer_id,
                                         const char* data, const uint64_t len) {
    lock_.lock();
    // check whether this peer_id exists
    if (id_to_connections.find(peer_id) == id_to_connections.end()) {
        CROW_LOG_WARNING << peer_id << " does not exist";
    }
    auto connection = id_to_connections[peer_id];
    lock_.unlock();
    connection->send_binary(string(data, len));
}
void MasterController::run(uint16_t listen_port) {
    std::thread([this]() {
        std::lock_guard<mutex>block_lock(lock_);
        for (auto p : id_to_connections) {
            sendMessageToPeer(p.first, "response");
        }
    }).detach();
    app_.port(listen_port).multithreaded().run();
}
