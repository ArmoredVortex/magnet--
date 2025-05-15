#pragma once

#include <string>
#include <vector>

namespace torrent
{
    struct infoStruct
    {
        u_int64_t length;
        std::string name;
        u_int64_t pieceLength;
        std::string pieces;
    };

    struct TorrentMeta
    {
        std::string announce;
        infoStruct info;
        std::string infoHash;
    };
}