#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <chrono>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include <iostream>
#include <sys/select.h>
#include <netdb.h>
#include <errno.h>
#include <bitset>
#include <sys/poll.h>
#include <limits>
#include <optional>

#include "peer.hpp"
#include "torrent.hpp"
#include <sstream>

constexpr int READ_TIMEOUT = 3000;
constexpr int CONNECT_TIMEOUT = 1; // seconds

int bytesToInt(std::string bytes)
{
    // FIXME: Use bitwise operation to convert
    std::string binStr;
    long byteCount = bytes.size();
    for (int i = 0; i < byteCount; i++)
        binStr += std::bitset<8>(bytes[i]).to_string();
    return stoi(binStr, 0, 2);
}

bool setSocketBlocking(int sock, bool blocking)
{
    if (sock < 0)
        return false;
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1)
        return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return fcntl(sock, F_SETFL, flags) == 0;
}

int createConnection(const std::string &ip, int port)
{
    struct addrinfo hints{}, *res, *p;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;     // AF_UNSPEC lets it find IPv4 or IPv6
    hints.ai_flags = AI_NUMERICHOST; // Since you have an IP address, not hostname

    std::string portStr = std::to_string(port);

    int ret = getaddrinfo(ip.c_str(), portStr.c_str(), &hints, &res);
    if (ret != 0)
        throw std::runtime_error("getaddrinfo failed: " + std::string(gai_strerror(ret)));

    int sock = -1;
    for (p = res; p != nullptr; p = p->ai_next)
    {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock < 0)
            continue;

        if (!setSocketBlocking(sock, false))
        {
            close(sock);
            continue;
        }

        int result = connect(sock, p->ai_addr, p->ai_addrlen);
        if (result < 0 && errno != EINPROGRESS)
        {
            close(sock);
            continue;
        }

        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(sock, &writefds);

        timeval timeout{};
        timeout.tv_sec = CONNECT_TIMEOUT;

        result = select(sock + 1, nullptr, &writefds, nullptr, &timeout);
        if (result <= 0)
        {
            close(sock);
            continue;
        }

        int so_error = 0;
        socklen_t len = sizeof(so_error);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
        if (so_error != 0)
        {
            close(sock);
            continue;
        }

        if (!setSocketBlocking(sock, true))
        {
            close(sock);
            continue;
        }

        freeaddrinfo(res);
        return sock; // success
    }

    freeaddrinfo(res);
    throw std::runtime_error("Failed to connect to " + ip + ":" + std::to_string(port));
}

std::string buildHandshakeMessage(const std::string &infoHashRaw)
{
    const std::string protocol = "BitTorrent protocol";
    std::stringstream buffer;
    buffer << (char)protocol.length();
    buffer << protocol;
    std::string reserved;
    for (int i = 0; i < 8; i++)
        reserved.push_back('\0');
    buffer << reserved;
    buffer << infoHashRaw;                       // Assuming infoHashRaw is already in the correct format
    std::string peerId = "-TR0001-070106001337"; // Example peer ID, you can generate a random one
    buffer << peerId;
    return buffer.str();
}

void sendData(const int sock, const std::string &data)
{
    int n = data.length();
    char buffer[n];
    for (int i = 0; i < n; i++)
        buffer[i] = data[i];

    int res = send(sock, buffer, n, 0);
    if (res < 0)
        throw std::runtime_error("Failed to write data to socket " + std::to_string(sock));
}

std::string receiveData(const int sock, uint32_t bufferSize)
{

    std::string reply;

    // If buffer size is not specified, read the first 4 bytes of the message
    // to obtain the total length of the response.
    if (!bufferSize)
    {
        struct pollfd fd;
        int ret;
        fd.fd = sock;
        fd.events = POLLIN;
        ret = poll(&fd, 1, READ_TIMEOUT);

        long bytesRead;
        const int lengthIndicatorSize = 4;
        char buffer[lengthIndicatorSize];
        switch (ret)
        {
        case -1:
            throw std::runtime_error("Read failed from socket " + std::to_string(sock));
        case 0:
            throw std::runtime_error("Read timeout from socket " + std::to_string(sock));
        default:
            bytesRead = recv(sock, buffer, sizeof(buffer), 0);
        }
        if (bytesRead != lengthIndicatorSize)
            return reply;

        std::string messageLengthStr;
        for (char i : buffer)
            messageLengthStr += i;
        uint32_t messageLength = bytesToInt(messageLengthStr);
        bufferSize = messageLength;
    }

    // If the buffer size is greater than uint16_t max, a segfault will
    // occur when initializing the buffer
    if (bufferSize > std::numeric_limits<uint16_t>::max())
        throw std::runtime_error("Received corrupted data [Received buffer size greater than 2 ^ 16 - 1]");

    char buffer[bufferSize];
    // memset(buffer, 0, bufferSize);
    // Receives reply from the host
    // Keeps reading from the buffer until all expected bytes are received
    long bytesRead = 0;
    long bytesToRead = bufferSize;
    // If not all expected bytes are received within the period of time
    // specified by READ_TIMEOUT, the read process will stop.
    auto startTime = std::chrono::steady_clock::now();
    do
    {
        auto diff = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration<double, std::milli>(diff).count() > READ_TIMEOUT)
        {
            throw std::runtime_error("Read timeout from socket " + std::to_string(sock));
        }
        bytesRead = recv(sock, buffer, bufferSize, 0);

        if (bytesRead <= 0)
            throw std::runtime_error("Failed to receive data from socket " + std::to_string(sock));
        bytesToRead -= bytesRead;
        for (int i = 0; i < bytesRead; i++)
            reply.push_back(buffer[i]);
    } while (bytesToRead > 0);

    return reply;
}

bool validateHandshake(const std::string &response, const std::string &expectedInfoHash)
{
    if (response.size() != 68)
    {
        std::cerr << "Invalid handshake size: expected 68, got " << response.size() << std::endl;
        return false;
    }

    uint8_t pstrlen = static_cast<uint8_t>(response[0]);
    if (pstrlen != 19)
    {
        std::cerr << "Invalid protocol length: expected 19, got " << (int)pstrlen << std::endl;
        return false;
    }

    std::string pstr = response.substr(1, 19);
    if (pstr != "BitTorrent protocol")
    {
        std::cerr << "Invalid protocol string: " << pstr << std::endl;
        return false;
    }

    std::string infoHash = response.substr(28, 20); // starts at offset 28

    if (infoHash != expectedInfoHash)
    {
        std::cerr << "Info hash mismatch" << std::endl;
        return false;
    }

    return true;
}

std::optional<int> connectAndHandshake(const Peer &peer, const std::string &infoHashRaw)
{
    try
    {
        int sock = createConnection(peer.ip, peer.port);
        std::cout << "Connected to " << peer.ip << ":" << peer.port << std::endl;

        std::string handshake = buildHandshakeMessage(infoHashRaw); // You implement this

        sendData(sock, handshake);

        // Receive handshake reply
        std::string response = receiveData(sock, handshake.size());

        if (!validateHandshake(response, infoHashRaw))
        {
            std::cerr << "Handshake validation failed for " << peer.ip << std::endl;
            close(sock);
            return std::nullopt;
        }

        return sock; // leave socket open for further use
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error connecting/handshaking with " << peer.ip << ":" << peer.port << " - " << e.what() << '\n';
        return std::nullopt;
    }
}

void sendInterested(int sockfd)
{
    uint32_t len = htonl(1); // message length
    uint8_t id = 2;          // 'interested' message ID

    std::vector<uint8_t> msg(5);
    memcpy(msg.data(), &len, 4);
    msg[4] = id;

    send(sockfd, msg.data(), 5, 0);
}

bool waitForUnchoke(int sockfd)
{
    while (true)
    {
        uint32_t len;
        if (recv(sockfd, &len, 4, MSG_WAITALL) != 4)
            return false;
        len = ntohl(len);
        if (len == 0)
            continue; // keep-alive

        uint8_t id;
        if (recv(sockfd, &id, 1, MSG_WAITALL) != 1)
            return false;

        if (id == 1)
            return true; // unchoke
        else
        {
            // read and discard the rest of the payload
            std::vector<uint8_t> payload(len - 1);
            if (recv(sockfd, payload.data(), len - 1, MSG_WAITALL) != len - 1)
                return false;
        }
    }
}

void sendRequest(int sockfd, int pieceIndex, int begin, int length)
{
    uint32_t len = htonl(13);
    uint8_t id = 6; // request

    std::vector<uint8_t> msg(17);
    memcpy(msg.data(), &len, 4);
    msg[4] = id;
    *reinterpret_cast<uint32_t *>(&msg[5]) = htonl(pieceIndex);
    *reinterpret_cast<uint32_t *>(&msg[9]) = htonl(begin);
    *reinterpret_cast<uint32_t *>(&msg[13]) = htonl(length);

    send(sockfd, msg.data(), 17, 0);
}

bool receivePiece(int sockfd, std::vector<uint8_t> &outData)
{
    uint32_t len;
    if (recv(sockfd, &len, 4, MSG_WAITALL) != 4)
        return false;
    len = ntohl(len);

    uint8_t id;
    if (recv(sockfd, &id, 1, MSG_WAITALL) != 1)
        return false;
    if (id != 7)
        return false; // not a piece

    // Read piece index, begin offset, and block data
    uint32_t index, begin;
    if (recv(sockfd, &index, 4, MSG_WAITALL) != 4)
        return false;
    if (recv(sockfd, &begin, 4, MSG_WAITALL) != 4)
        return false;
    index = ntohl(index);
    begin = ntohl(begin);

    int blockLen = len - 9;
    outData.resize(blockLen);
    if (recv(sockfd, outData.data(), blockLen, MSG_WAITALL) != blockLen)
        return false;

    return true;
}
