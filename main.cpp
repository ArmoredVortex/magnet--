#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "bencode.hpp"
#include "bencodeParser.hpp"
#include "torrent.hpp"
#include "tracker.hpp"
#include "peer.hpp"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    torrent::TorrentMeta torrentFile = parseTorrentMeta(content);

    // std::cout << torrentFile.infoHash << std::endl;

    std::vector<Peer> peers = getPeers(torrentFile);

    if (peers.empty())
    {
        std::cerr << "No peers found." << std::endl;
        return 1;
    }
    else
    {
        std::cout << peers.size() << " peer(s) found!" << std::endl;
    }

    // for (const auto &peer : peers)
    // {
    //     std::cout << "Peer IP: " << peer.ip << ", Port: " << peer.port << std::endl;
    // }

    for (const auto &peer : peers)
    {
        std::cout << "Trying " << peer.ip << ":" << peer.port << std::endl;
        if (connectAndHandshake(peer, torrentFile.infoHashRaw))
        {
            std::cout << "\033[32mConnected and handshake successful with \033[0m" << peer.ip << std::endl;
        }
        else
        {
            std::cout << "Failed to connect/handshake with " << peer.ip << std::endl;
        }
    }

    return 0;
}