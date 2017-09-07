#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

#include <mysql/plugin.h>
#include <mysql/plugin_audit.h>
#include <string.h>
#include <channel.h>

#ifndef PSI_NO_INSTRUMENTATION
#define PSI_NO_INSTRUMENTATION 0
#endif

#define MAX_SEND_RETRIES 3

static channel_t* connection = NULL;

/* -----------------------
 * Plugin system variables
 * -----------------------
 */
static char* an_service_endpoint_host = NULL;
static unsigned int an_service_endpoint_port = 12345;
static char an_service_endpoint_registered = 0;
/* ------------------------ */

static char async_notifications_status[] = "Ok";

static int check_registration_status(MYSQL_THD thd MY_ATTRIBUTE((unused)), 
        struct st_mysql_sys_var* var MY_ATTRIBUTE((unused)), void* temp_storage, 
        struct st_mysql_value* value)
{
    int status = value->val_int(value, (long long*)temp_storage);

    switch (status) {
        case 0:
        case 1:
            return 0;
        default:
            return 1;
    }

    return 0;
}

static void update_registration_status(MYSQL_THD thd MY_ATTRIBUTE((unused)), 
        struct st_mysql_sys_var* var MY_ATTRIBUTE((unused)), void* var_ptr MY_ATTRIBUTE((unused)), 
        const void* temp_storage)
{
    long long* statusp = (long long*)temp_storage;
    if (*statusp == 0) {
        an_service_endpoint_registered = 0;

        if (connection != NULL) {
            channel_destroy(connection);
            connection = NULL;
        }
    } else {
        an_service_endpoint_registered = 1;

        if (connection == NULL) {
            char port[sizeof(unsigned int) * 8 + 1];
            memset(&port, 0, sizeof(port));
            sprintf(port, "%u", an_service_endpoint_port);
            connection = channel_create(an_service_endpoint_host, port);
        }
    }
}

static MYSQL_SYSVAR_STR(service_endpoint_host, an_service_endpoint_host,
        PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_MEMALLOC,
        "Hostname of endpoint to which notifications are sent",
        NULL, NULL, "127.0.0.1");

static MYSQL_SYSVAR_UINT(service_endpoint_port, an_service_endpoint_port,
        PLUGIN_VAR_OPCMDARG,
        "Port number of endpoint to which notifications are sent",
        NULL, NULL, 12345, 1, 65535, 0);

static MYSQL_SYSVAR_BOOL(service_endpoint_registered, an_service_endpoint_registered,
        PLUGIN_VAR_NOCMDOPT,
        "Service endpoint registration status",
        check_registration_status, update_registration_status, 0);

static int async_notifications_notify(MYSQL_THD thd MY_ATTRIBUTE((unused)), 
        mysql_event_class_t event_class,
        const void *event)
{
    if (event_class == MYSQL_AUDIT_TABLE_ACCESS_CLASS) {
        size_t buf_size;
        const struct mysql_event_table_access *event_table = 
            (const struct mysql_event_table_access *)event;

        // todo fprintf
        const char* format = "{ 'schema': '%s', 'table': '%s', 'type': %d, 'ts': %ld%ld }%s";
        
        const char* separator = "\n\n";
        const char* database = event_table->table_database.str;
        const char* table = event_table->table_name.str;
        struct timeval now;
        if (gettimeofday(&now, (void*)0) != 0) {
            return errno;
        }

        // 12 = length of conversion format strings
        // 10 = max length of a signed integer when interpreted as string
        // 19 = max length of unsigned long long when interpreted as string
        buf_size = (strlen(format) - 12) + strlen(database) + strlen(table) + 10 + 19 + 1;
        char buf[buf_size];
        memset(&buf, 0, buf_size);

        snprintf(buf, sizeof(buf), format, database, table, event_table->event_subclass, 
                (long int)now.tv_sec, (long int)now.tv_usec, separator);

        if (an_service_endpoint_registered) {
            if (channel_put(connection, buf) == -1) {
                int retries = 0;
                while (connection->state == SEND_FAIL) {
                    if (retries >= MAX_SEND_RETRIES) { break; }

                    channel_retry(connection, buf);
                    retries++;
                }

                if (retries >= MAX_SEND_RETRIES) {
                    // log something, close the connection and unregister the endpoint.
                    channel_destroy(connection);
                    connection = NULL;
                    an_service_endpoint_registered = 0;
                }
            }
        }
    }

    return 0;
}

/* Audit plugin descriptor */
static struct st_mysql_audit async_notifications_descriptor =
{
    MYSQL_AUDIT_INTERFACE_VERSION,                      /* interface version */
    NULL,                                               /* release thread function */
    async_notifications_notify,                         /* notify function */
    {
        0, /* MYSQL_AUDIT_GENERAL_ALL */
        0, /* MYSQL_AUDIT_CONNECTION_ALL */
        0, /* MYSQL_AUDIT_PARSE_ALL */
        0, /* currently not supported */
        (unsigned long) MYSQL_AUDIT_TABLE_ACCESS_ALL,    /* subscribe to table access events */
        0, /* MYSQL_AUDIT_GLOBAL_VARIABLE_ALL */
        0, /* MYSQL_AUDIT_SERVER_STARTUP_ALL */
        0, /* MYSQL_AUDIT_SERVER_SHUTDOWN_ALL */
        0, /* MYSQL_AUDIT_QUERY_ALL */
        0  /* MYSQL_AUDIT_STORED_PROGRAM_ALL */
    }
};

static int async_notifications_init(void *arg MY_ATTRIBUTE((unused)))
{
    return 0;
}

static int async_notifications_deinit(void *arg MY_ATTRIBUTE((unused)))
{
    if (connection) {
        channel_destroy(connection);
        connection = NULL;
    }

    return 0;
}

static struct st_mysql_show_var simple_status[] =
{
    { "Async_Notifications_Status",
        async_notifications_status,
        SHOW_CHAR_PTR, SHOW_SCOPE_GLOBAL
    },
    { 0, 0, 0, SHOW_SCOPE_GLOBAL }
};

static struct st_mysql_sys_var* system_variables[] = 
{
    MYSQL_SYSVAR(service_endpoint_host),
    MYSQL_SYSVAR(service_endpoint_port),
    MYSQL_SYSVAR(service_endpoint_registered),
    NULL
};

/* Generic plugin descriptor */
mysql_declare_plugin(async_notifications)
{
    MYSQL_AUDIT_PLUGIN,                             /* type                             */
    &async_notifications_descriptor,                /* descriptor                       */
    "ASYNC_NOTIFICATIONS",                          /* name                             */
    "houthacker",                                   /* author                           */
    "Async notifications of insert,update,delete",  /* description                      */
    PLUGIN_LICENSE_GPL,                             /* license                          */
    async_notifications_init,                       /* init function (when laded)       */
    async_notifications_deinit,                     /* deinit function (when unloaded)  */
    0x0001,                                         /* version                          */
    simple_status,                                  /* status variables                 */
    system_variables,                               /* system variables                 */
    NULL,                                           /* reserved for dependency checks   */
    0,                                              /* flags for plugin                 */
}
mysql_declare_plugin_end;

