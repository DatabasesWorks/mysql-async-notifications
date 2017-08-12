#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <channel.h>

channel create_channel(const char* hostname, const char* servname)
{
    struct addrinfo hints, *res;
    int channel = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(hostname, servname, &hints, &res);
    channel = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    fcntl(channel, F_SETFL, O_NONBLOCK);

    if (connect(channel, res->ai_addr, res->ai_addrlen)) {
        return channel;
    }

    return 0;
}

int destroy_channel(channel channel)
{
    return close(channel);
}

int channel_put(const channel channel, const char* message)
{
    int len = strlen(message);
    return send((const int)channel, message, len, 0);
}
