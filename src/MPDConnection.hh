#include <mpd/connection.h>
#include <mpd/client.h>

class MPDConnection
{
public:
    MPDConnection()
    {
        reconnect();
    }

    ~MPDConnection()
    {
        if (mConn)
            mpd_connection_free(mConn);
    }

    bool add(std::string const &url)
    {
        return retry([&]() {
            auto res = mpd_run_add(mConn, url.c_str());
            if (!res)
                std::cerr << "Could not add " << url << "\n"
                          << mpd_connection_get_error_message(mConn) << std::endl;
            return res;
        });
    }

    bool clear()
    {
        return retry([&]() {
            return mpd_run_clear(mConn);
        });
    }

    bool play()
    {
        return retry([&]() {
            return mpd_run_play(mConn);
        });
    }

protected:
    mpd_connection *mConn = nullptr;
    void reconnect()
    {
        if (mConn)
            mpd_connection_free(mConn);
        mConn = mpd_connection_new(nullptr, 0, 0);
        if (mpd_connection_get_error(mConn) != MPD_ERROR_SUCCESS)
        {
            throw new std::runtime_error(mpd_connection_get_error_message(mConn));
        }
        mpd_connection_set_keepalive(mConn, true);
    }

    template <class FN, typename... Args>
    bool retry(FN &&fn, Args &&...args)
    {
        int tries = 3;
        do
        {
            auto success = fn(std::forward<Args>(args...)...);
            if (success)
                return true;
            reconnect();
        } while (tries--);
        return false;
    }
};