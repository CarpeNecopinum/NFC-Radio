#include <iostream>

#include <NFCListener.hh>
#include <MPDConnection.hh>
#include <ytdl_interface.hh>

#include <thread>
#include <chrono>

int main(void)
{
    MPDConnection mpd;
    std::cout << "Connected to MPD" << std::endl;
    NFCListener listener;
    std::cout << "Connected to NFC device" << std::endl;

    listener.cardFound.on([](const Card &card) {
        std::cout << "Card Found: " << card.name << " (" << card.uid << ")\n";
    });

    listener.message.on([&](const Message &m) {
        std::cout << "Message Found:\n"
                  << "   Type:    " << m.type << "\n"
                  << "   ID:      " << m.id << "\n"
                  << "   Proto:   " << uint32_t(m.uri_proto) << "\n"
                  << "   Payload: " << m.content << "\n";

        std::cout << m.content.rfind("music.youtube.com", 0) << "\n";

        if (m.uri_proto == URIProtocol::HTTP || m.uri_proto == URIProtocol::HTTPS)
        {
            std::cout << "Message was HTTP(s)\n";
            if (m.content.rfind("music.youtube.com", 0) == 0)
            {
                std::cout << "Identified youtube-music link \n";
                mpd.add("loading.mp3");
                mpd.play();
                auto urls = ytdl::extractAudioURLs("http://" + m.content);
                std::cout << "Extracted " << urls.size() << " song URLs\n";
                mpd.clear();
                for (auto const &url : urls)
                    mpd.add(url);
                mpd.play();
            }
        }
    });

    listener.cardRemoved.on([&](const Card &card) {
        std::cout << "Card Removed. \n";
        mpd.clear();
    });

    for (;;)
    {
        listener.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}