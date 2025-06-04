#include "peer.hpp"
#include "torrent.hpp"
#include <iostream>
#include <fstream>
#include <openssl/sha.h>

bool downloadFullFile(PeerConnection &pc, const torrent::TorrentMeta &torrentMeta);