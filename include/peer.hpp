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
#include <optional>
#include <vector>

struct Peer
{
    std::string ip;
    u_int64_t port;
    // std::string peer_id;
};

int createConnection(const std::string &ip, int port);
std::optional<int> connectAndHandshake(const Peer &peer, const std::string &infoHashRaw);

struct PeerConnection
{
    Peer peer;  // IP, port
    int sockfd; // active socket
    bool choked = true;
    bool interested = false;
    std::vector<bool> bitfield; // which pieces they have
};

void sendInterested(int sockfd);
bool waitForUnchoke(int sockfd);
void sendRequest(int sockfd, int pieceIndex, int begin, int length);
bool receivePiece(int sockfd, std::vector<uint8_t> &outData);
