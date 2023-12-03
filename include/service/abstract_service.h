#ifndef __ABSTRACT_SERVICE_H__
#define __ABSTRACT_SERVICE_H__
#include <cstdlib>
#include <string>
class AbstractController;
class AbstractService {
   public:
    AbstractService(AbstractController* controller) : controller_(controller){};
    virtual ~AbstractService() = default;

    /// @brief This function will be called during initializaion. You can do
    /// some init work in this function.
    virtual void onInit() = 0;

    /// @brief This function will be called when a new message is received.
    /// @param peer_id the id of the sender. N.B. master's id is always marco
    /// MASTER_ID, and backup master's id is always BACKUP_MASTER_ID
    /// @param message the message which is received. When the message is text
    /// format, the string is the text; when the message is binary data, the
    /// message stores the binary data, and you can access it with
    /// message.data(), and the length of the data is messsage.size();
    /// @param is_binary Whether this is a binary data
    virtual void onNewMessage(std::string peer_id, const std::string& message,
                              bool is_binary) = 0;

    /// @brief This function will be called when a peer establish a connection.
    /// @param peer_id the peer's id who just established the connection. N.B.
    /// master's id is always marco MASTER_ID, and backup master's id is always
    /// BACKUP_MASTER_ID
    virtual void onConnectionEstablished(const std::string peer_id) = 0;

    /// @brief This function will be called when a peer closes a connection.
    /// @param peer_id the peer's id who just closed the connection. N.B.
    /// master's id is always marco MASTER_ID, and backup master's id is always
    /// BACKUP_MASTER_ID
    virtual void onConnectionTerminated(const std::string peer_id) = 0;

    /// @brief This function has been implemented. It is used to send a text
    /// message to a peer, and you can call this function.
    /// @param peer_id the peer's id to whom you want to send the message.
    /// @param message the message you want to send
    virtual void sendMessageToPeer(const std::string& peer_id,
                                   const std::string& message);

    /// @brief This function has been implemented. It is used to send a binary
    /// message to a peer, and you can call this function.
    /// @param peer_id the peer's id to whom you want to send the message.
    /// @param data the buffer containing the binary message you want to send
    /// @param len the length of the binary message you want to send

    virtual void sendMessageToPeer(const std::string& peer_id, const char* data,
                                   const uint64_t len);

   protected:
    AbstractController* controller_;
};

#endif