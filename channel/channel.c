#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <channel.h>

int get_socket_fd(const char* hostname, const char* servname);

channel_t* channel_create(const char* hostname, const char* servname)
{
    channel_t* channel = (channel_t*)calloc(1, sizeof(channel_t));
    if (channel) {
        channel->fd = get_socket_fd(hostname, servname);
        channel->hostname = strdup(hostname);
        channel->servname = strdup(servname);
    }
    
    return channel;
}

int channel_destroy(channel_t* channel)
{
   if (channel) {
       close(channel->fd);
       free(channel->hostname);
       free(channel->servname);

       memset(channel, 0, sizeof(channel_t));
       free(channel);
   }

   return 0;
}

int channel_put(const channel_t* channel, const char* message)
{
    int result = send(channel->fd, message, strlen(message), 0);
    if (result == -1) {

        // If sending the message failed, check what was the error and based on that,
        // possibly reconnect and try again (once).
        // TODO add plugin variable an_client_registered (bool)
    }

    return result;
}

int get_socket_fd(const char* hostname, const char* servname)
{
    struct addrinfo hints, *res;

    int fd = -1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(hostname, servname, &hints, &res);
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (fd != -1) {
        fcntl(fd, F_SETFL, O_NONBLOCK);

        if (!connect(fd, res->ai_addr, res->ai_addrlen)) {
            fd = -1;
        }
    }

    return fd;
}
