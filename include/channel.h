#ifndef CHANNEL_H_DEFINED
#define CHANNEL_H_DEFINED

typedef int channel;

/**
 * Creates a new channel to send incoming audit events to.
 *
 * @return A connection to which messages can be sent.
 */
channel create_channel(const char* hostname, const char* servname);

/**
 * Destroys the provided channel.
 *
 * @return a status code indicating success or failure.
 */
int destroy_channel(channel channel);

/**
 * Sends the provided message to the given channel, completely ignoring any possible 
 * response.
 *
 * @return a status code indicating success or failure.
 */
int channel_put(const channel channel, const char* message);

#endif
