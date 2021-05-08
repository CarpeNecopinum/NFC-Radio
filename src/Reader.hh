#include <stdint.h>
#include <stddef.h>
#include <stdexcept>

class Reader
{
public:
    Reader(uint8_t *data, size_t size) : mData(data), mSize(size) {}
    void reset(size_t offset = 0) { mOffset = offset; }

    template <typename T>
    T read()
    {
        if (mOffset + sizeof(T) > mSize)
            throw std::range_error("Read beyond end of Reader buffer.");

        auto res = *(T *)(mData + mOffset);
        mOffset += sizeof(T);
        return res;
    }

    std::string read(size_t len)
    {
        if (mOffset + len > mSize)
            throw std::range_error("Read beyond end of Reader buffer.");
        auto result = std::string(mData + mOffset, mData + mOffset + len);
        mOffset += len;
        return result;
    }

    uint8_t *current() const { return mData + mOffset; }
    size_t rest() const { return mSize - mOffset; }

protected:
    uint8_t *mData;
    size_t mSize;
    size_t mOffset = 0;
};