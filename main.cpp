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
    std::vector<Peer> peers = getPeers(torrentFile);

    if (peers.empty())
    {
        std::cerr << "No peers found." << std::endl;
        return 1;
    }
    else
    {
        std::cout << peers.size() << " peers found!" << std::endl;
    }
    for (const auto &peer : peers)
    {
        std::cout << "IP: " << peer.ip << ", Port: " << peer.port << ", Peer ID: " << peer.peer_id << std::endl;
    }

    return 0;
}