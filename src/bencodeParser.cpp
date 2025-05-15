#include <string>
#include <unordered_map>

#include <iostream>

#include "torrent.hpp"
#include "bencode.hpp"
#include "TinySHA1.hpp"

torrent::TorrentMeta parseTorrentMeta(const std::string &content)
{
    torrent::TorrentMeta torrentFile;
    auto data = bencode::decode(content);

    std::string announce = std::get<std::string>(data["announce"]);
    torrentFile.announce = announce;

    const bencode::data info_node = data["info"];
    bencode::dict info = std::get<bencode::dict>(data["info"]);

    torrentFile.info.length = std::get<bencode::integer>(info["length"]);
    torrentFile.info.name = std::get<std::string>(info["name"]);
    torrentFile.info.pieceLength = std::get<bencode::integer>(info["piece length"]);
    torrentFile.info.pieces = std::get<std::string>(info["pieces"]);

    // Calculate the info hash
    std::string bencoded_info = bencode::encode(info_node);
    sha1::SHA1 s;
    s.processBytes(bencoded_info.c_str(), bencoded_info.size());
    uint32_t digest[5];
    s.getDigest(digest);
    char tmp[48];
    snprintf(tmp, 45, "%08x%08x%08x%08x%08x", digest[0], digest[1], digest[2], digest[3], digest[4]);
    torrentFile.infoHash = std::string(tmp, 40);

    return torrentFile;
}