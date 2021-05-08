#include <stdint.h>
#include <vector>
#include <algorithm>
#include <functional>

template <class EventT>
class EventEmitter
{
    using Handle = uint32_t;
    using Handler = std::function<void(EventT const &)>;

public:
    Handle on(Handler &&handler)
    {
        mHandlers.emplace_back(mNextHandle, std::move(handler));
        return mNextHandle++;
    }
    void off(Handle handle)
    {
        auto it = find_if(begin(mHandlers), end(mHandlers), [&](auto pair) { return pair.first == handle; });
        if (it != end(mHandlers))
            mHandlers.erase(it);
    }

    void trigger(EventT const &event)
    {
        for (auto const &hnd : mHandlers)
            hnd.second(event);
    }

private:
    Handle mNextHandle = 1;
    std::vector<std::pair<Handle, Handler>> mHandlers;
};