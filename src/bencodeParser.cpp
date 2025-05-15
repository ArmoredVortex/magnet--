#include <string>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <variant>

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
    bencode::data response = bencode::decode(content);
    std::vector<Peer> peers;

    if (!std::holds_alternative<bencode::dict>(response))
    {
        throw std::runtime_error("Expected top-level bencode dict");
    }

    bencode::dict response_dict = std::get<bencode::dict>(response);

    auto it = response_dict.find("peers");
    if (it == response_dict.end())
    {
        throw std::runtime_error("Missing 'peers' key");
    }

    const bencode::data &peers_field = it->second;

    if (std::holds_alternative<std::string>(peers_field))
    {
        std::cout << "Compact peer list detected!" << std::endl;

        const char *data = content.data();
        for (int i = 0; i < content.size(); i += 6)
        {
            Peer p;
            p.ip = std::to_string(static_cast<unsigned char>(data[i])) + "." +
                   std::to_string(static_cast<unsigned char>(data[i + 1])) + "." +
                   std::to_string(static_cast<unsigned char>(data[i + 2])) + "." +
                   std::to_string(static_cast<unsigned char>(data[i + 3]));
            p.port = (static_cast<unsigned char>(data[i + 4]) << 8) |
                     static_cast<unsigned char>(data[i + 5]);
            peers.push_back(p);
        }
    }
    else if (std::holds_alternative<std::vector<bencode::data>>(peers_field))
    {
        std::cout << "Non-compact peer list detected!" << std::endl;

        // Parse the non-compact peer list
        bencode::list peers_list = std::get<bencode::list>(peers_field);
        for (const auto &peer : peers_list)
        {
            auto decoded_peer = std::get<bencode::dict>(peer);
            Peer p;
            p.ip = std::get<std::string>(decoded_peer["ip"]);
            p.port = std::get<bencode::integer>(decoded_peer["port"]);
            // p.peer_id = std::get<std::string>(decoded_peer["peer id"]);
            peers.push_back(p);
        }
    }
    else
    {
        throw std::runtime_error("Unknown format in 'peers' field");
    }

    return peers;
}
