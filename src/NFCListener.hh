#pragma once
#include <EventEmitter.hh>
#include <string>
#include <nfc/nfc-types.h>
#include <freefare.h>

struct Message
{
    std::string type;
    std::string id;
    std::string content;
};

struct Card
{
    std::string name;
    std::string uid;
};

class NFCListener
{
public:
    NFCListener();
    EventEmitter<Card> cardFound;
    EventEmitter<Message> message;
    EventEmitter<Card> cardRemoved;
    void poll();

private:
    nfc_device *mDevice = nullptr;
    nfc_context *mContext = nullptr;
    nfc_target mTarget;
    Card mLastCard;

    void read_ultralight(MifareTag const &tag);
    void read_classic(MifareTag const &tag);
    void handle_body(uint8_t *data, size_t n);
    void handle_message(uint8_t *data, size_t n);
};