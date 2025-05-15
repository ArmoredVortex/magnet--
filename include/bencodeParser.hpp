#pragma once

#include <string>
#include "torrent.hpp"

torrent::TorrentMeta parseTorrentMeta(const std::string &content);