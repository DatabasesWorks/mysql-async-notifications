INCLUDE_DIRECTORIES("./include")

SET(ASYNC_NOTIFICATIONS_SOURCES
    channel/channel.c
    async_notifications.c
)

MYSQL_ADD_PLUGIN(async_notifications ${ASYNC_NOTIFICATIONS_SOURCES}
    MODULE_ONLY MODULE_OUTPUT_NAME "async_notifications")

