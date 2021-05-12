#include <NFCListener.hh>

#include <nfc/nfc.h>
#include <stdexcept>

#include <iostream>

#include <Reader.hh>

static MifareClassicKey default_keys[] = {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7},
    {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5},
    {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5},
    {0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd},
    {0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a},
    {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static bool brute_authenticate(MifareTag tag, MifareClassicBlockNumber block)
{
    for (size_t i = 0; i < (sizeof(default_keys) / sizeof(MifareClassicKey)); i++)
    {
        mifare_classic_connect(tag);
        if (mifare_classic_authenticate(tag, block, default_keys[i], MFC_KEY_B) == 0)
            return true;
        mifare_classic_connect(tag);
        if (mifare_classic_authenticate(tag, block, default_keys[i], MFC_KEY_A) == 0)
            return true;
    }
    return false;
}

NFCListener::NFCListener()
{
    nfc_init(&mContext);
    if (!mContext)
        throw std::runtime_error("Could not init NFC context.");

    auto n_tries = 10;
    do
    {
        mDevice = nfc_open(mContext, nullptr);
    } while (!mDevice && n_tries--);
    if (!mDevice)
        throw std::runtime_error("Could not open device.");

    auto init_success = nfc_initiator_init(mDevice) >= 0;
    if (!init_success)
        throw std::runtime_error("Could not init as initiator.");
}

void NFCListener::read_ultralight(MifareTag const &tag)
{
    const auto max_pages = 256;
    MifareUltralightPage pages[max_pages] = {};

    mifare_ultralight_connect(tag);

    // mifare_ultralight_read only does 16 pages, so we have to do the reading ourselves
    int page = 0;
    for (; page < max_pages; page += 4)
    {
        uint8_t cmd[] = {0x30, page};
        auto res = (uint8_t *)(pages + page);
        auto reply = nfc_initiator_transceive_bytes(mDevice, cmd, sizeof(cmd), res, 16, 0);
        if (reply == -1)
            break;
    }
    page -= 4;

    std::cout << page << std::endl;

    handle_body((uint8_t *)(pages + 4), page * sizeof(MifareUltralightPage));
}

void NFCListener::read_classic(MifareTag const &tag)
{
    brute_authenticate(tag, 0);
    auto mad = mad_read(tag);
    if (!mad)
        return;

    uint8_t buffer[4096];
    auto bytes_read = mifare_application_read(tag, mad, mad_nfcforum_aid, buffer, sizeof(buffer), mifare_classic_nfcforum_public_key_a, MFC_KEY_A);

    handle_body(buffer, bytes_read);
}

void NFCListener::poll()
{
    MifareTag *tags = freefare_get_tags(mDevice);
    if (!tags || !tags[0])
    {
        if (mLastCard.uid.length())
            cardRemoved.trigger(mLastCard);
        mLastCard = {};
        return;
    }

    // only interested in one tag at a time
    auto tag = tags[0];
    if (mLastCard.uid == freefare_get_tag_uid(tag))
        return;

    mLastCard = {freefare_get_tag_friendly_name(tag),
                 freefare_get_tag_uid(tag)};
    cardFound.trigger(mLastCard);

    auto type = freefare_get_tag_type(tag);
    switch (type)
    {
    case ULTRALIGHT_C:
    case ULTRALIGHT:
        read_ultralight(tag);
        break;
    case CLASSIC_1K:
    case CLASSIC_4K:
        read_classic(tag);
        break;
    }
}

void NFCListener::handle_body(uint8_t *data, size_t n)
{
    auto f = fopen("dump.bin", "w");
    fwrite(data, n, 1, f);
    fclose(f);

    uint8_t tlv_type;
    uint16_t tlv_data_len;
    uint8_t *tlv_data;
    uint8_t const *pbuffer = data;

    for (;;)
    {
        if (pbuffer >= data + n)
            break;
        tlv_data = tlv_decode(pbuffer, &tlv_type, &tlv_data_len);
        if (!tlv_data)
            break;

        switch (tlv_type)
        {
        case 0x03: // Message TLV - good
            handle_message(tlv_data, tlv_data_len);
            break;
        case 0x00: // NULL TLV - ignore
        case 0xFD: // proprietary TLV - ignore
            break;
        case 0xFE: // Terminator TLV - bad
        default:   // Invalid TLV - bad
            return;
            break;
        }
        pbuffer += tlv_record_length(pbuffer, nullptr, nullptr);
    }
}

struct ndef_header
{
    uint8_t tnf : 3;
    bool il : 1;
    bool sr : 1;
    bool cf : 1;
    bool me : 1;
    bool mb : 1;
    uint8_t type_len;
};

void NFCListener::handle_message(uint8_t *data, size_t n)
{
    auto reader = Reader(data, n);
    auto header = reader.read<ndef_header>();

    auto len_payload = header.sr ? reader.read<uint8_t>() : reader.read<uint32_t>();
    auto len_id = header.il ? reader.read<uint8_t>() : 0u;

    auto type = reader.read(header.type_len);
    auto id = len_id ? reader.read(len_id) : "";
    auto proto = URIProtocol{};
    if (type == "U")
    {
        proto = reader.read<URIProtocol>();
        len_payload -= 1;
    }
    auto payload = reader.read(len_payload);

    message.trigger({std::move(type),
                     std::move(id),
                     proto,
                     std::move(payload)});

    if (reader.rest())
        handle_message(reader.current(), reader.rest());
}