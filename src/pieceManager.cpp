#include <vector>

struct PeerConnection
{
    Peer peer;  // IP, port
    int sockfd; // active socket
    bool choked = true;
    bool interested = false;
    std::vector<bool> bitfield; // which pieces they have
    // Maybe track last message timestamp, downloading state, etc.
};
