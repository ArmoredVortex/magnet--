#pragma once

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

torrent::TorrentMeta parseTorrentMeta(const std::string &content);

std::vector<Peer> parsePeerList(const std::string &content);