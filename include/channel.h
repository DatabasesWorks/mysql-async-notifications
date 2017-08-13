#ifndef CHANNEL_H_DEFINED
#define CHANNEL_H_DEFINED

struct channel {
    int fd;
    char* hostname;
    char* servname;
};
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
int channel_put(const channel_t* channel, const char* message);

#endif
