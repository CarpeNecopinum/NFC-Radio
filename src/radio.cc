#include <iostream>

#include <NFCListener.hh>

#include <thread>
#include <chrono>

int main(void)
{
    NFCListener listener;

    listener.cardFound.on([](const Card &card) {
        std::cout << "Card Found: " << card.name << " (" << card.uid << ")\n";
    });

    listener.message.on([](const Message &m) {
        std::cout << "Message Found:\n"
                  << "   Type:    " << m.type << "\n"
                  << "   ID:      " << m.id << "\n"
                  << "   Payload: " << m.content << "\n";
    });

    listener.cardRemoved.on([](const Card &card) {
        std::cout << "Card Removed. \n";
    });

    for (;;)
    {
        listener.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}