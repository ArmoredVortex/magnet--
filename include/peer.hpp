#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include <iostream>
#include <sys/select.h>

struct Peer
{
    std::string ip;
    u_int64_t port;
    // std::string peer_id;
};

int createConnection(const std::string &ip, int port);
bool connectAndHandshake(const Peer &peer, const std::string &infoHashRaw);