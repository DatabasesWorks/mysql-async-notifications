#ifndef CHANNEL_H_DEFINED
#define CHANNEL_H_DEFINED

enum channel_state { CONNECTED, SEND_FAIL };

struct channel {
    int fd;
    char* hostname;
    char* servname;
    enum channel_state state;
};

typedef enum channel_state channel_state_t;
typedef struct channel channel_t;

/**
 * Creates a new channel to send incoming audit events to.
 *
 * @return A connection to which messages can be sent.
 */
channel_t* channel_create(const char* hostname, const char* servname);

/**
 * Destroys the provided channel.
 *
 * @return a status code indicating success or failure.
 */
int channel_destroy(channel_t* channel);

/**
 * Sends the provided message to the given channel, completely ignoring any possible 
 * response.
 *
 * @return a status code indicating success or failure.
 */
int channel_put(channel_t* channel, const char* message);

/**
 * Retries sending the given message, reconnecting to the receiving host
 * if the channel is in a erroneous state.
 * If the channel is in the CONNECTED state, this method has the same
 * effect as channel_put.
 *
 * @return a status code indicating success or failure.
 */
int channel_retry(channel_t* channel, const char* message);

/**
 * Creates a new socket, connects and returns its file descriptor number.
 * On failure, -1 is returned and errno is set.
 *
 * @return the file descriptor, or -1 on failure.
 */
int channel_get_socket_fd(const char* hostname, const char* servname);

#endif
