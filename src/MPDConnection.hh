#include <mpd/connection.h>
#include <mpd/client.h>

class MPDConnection
{
public:
    MPDConnection()
    {
        mConn = mpd_connection_new(nullptr, 0, 0);
        if (mpd_connection_get_error(mConn) != MPD_ERROR_SUCCESS)
        {
            throw new std::runtime_error(mpd_connection_get_error_message(mConn));
        }
    }

    ~MPDConnection()
    {
        if (mConn)
            mpd_connection_free(mConn);
    }

    bool add(std::string const &url)
    {
        auto res = mpd_run_add(mConn, url.c_str());
        if (!res)
            std::cerr << "Could not add " << url << "\n"
                      << mpd_connection_get_error_message(mConn) << std::endl;
        return res;
    }

    bool clear()
    {
        return mpd_run_clear(mConn);
    }

    bool play()
    {
        return mpd_run_play(mConn);
    }

protected:
    mpd_connection *mConn;
};