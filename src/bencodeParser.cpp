#include <string>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cctype>

#include "torrent.hpp"
#include "bencode.hpp"
#include "TinySHA1.hpp"
#include "peer.hpp"

std::string percentEncodeHexString(const std::string &hexString)
{
    std::string result;
    for (size_t i = 0; i < hexString.length(); i += 2)
    {
        if (i + 1 < hexString.length())
        {
            result += "%" + hexString.substr(i, 2);
        }
    }
    return result;
}

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
    torrentFile.infoHash = percentEncodeHexString(std::string(tmp, 40));

    return torrentFile;
}

std::vector<Peer> parsePeerList(const std::string &content)
{
    bencode::data data = bencode::decode(content);
    const bencode::data peers_node = data["peers"];
    bencode::list peers_list = std::get<bencode::list>(peers_node);

    std::vector<Peer> peers;
    for (const auto &peer : peers_list)
    {
        auto decoded_peer = std::get<bencode::dict>(peer);
        Peer p;
        p.ip = std::get<std::string>(decoded_peer["ip"]);
        p.port = std::get<bencode::integer>(decoded_peer["port"]);
        p.peer_id = std::get<std::string>(decoded_peer["peer id"]);
        peers.push_back(p);
    }
    return peers;
}