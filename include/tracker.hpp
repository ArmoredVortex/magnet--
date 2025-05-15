#pragma once

#include <string>
#include <vector>

#include "peer.hpp"
#include "torrent.hpp"
#include "httplib.h"

std::vector<Peer> getPeers(torrent::TorrentMeta torrentFile);
