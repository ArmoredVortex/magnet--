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
    std::cout << torrentFile.announce << std::endl;
    std::cout << torrentFile.infoHash << std::endl;
    std::cout << torrentFile.info.length << std::endl;
    // std::vector<Peer> peers = tracker::getPeers(torrentFile.announce);

    // if (peers.empty())
    // {
    //     std::cerr << "No peers found." << std::endl;
    //     return 1;
    // }
    return 0;
}