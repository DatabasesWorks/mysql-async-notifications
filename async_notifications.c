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

static channel connection;

/* -----------------------
 * Plugin system variables
 * -----------------------
 */
static char* an_service_endpoint_host = NULL;
static unsigned int an_service_endpoint_port = 12345;
/* ------------------------ */

static char async_notifications_status[] = "Ok";

static MYSQL_SYSVAR_STR(service_endpoint_host, an_service_endpoint_host,
        PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_MEMALLOC,
        "Hostname of endpoint to which notifications are sent",
        NULL, NULL, "127.0.0.1");

static MYSQL_SYSVAR_UINT(service_endpoint_port, an_service_endpoint_port,
        PLUGIN_VAR_OPCMDARG,
        "Port number of endpoint to which notifications are sent",
        NULL, NULL, 12345, 1, 65535, 0);

static int async_notifications_notify(MYSQL_THD thd, 
        mysql_event_class_t event_class,
        const void *event)
{
    if (event_class == MYSQL_AUDIT_TABLE_ACCESS_CLASS) {
        const struct mysql_event_table_access *event_table = 
            (const struct mysql_event_table_access *)event;

        // todo fprintf
        const char* format = "{ 'schema': '%s', 'table': '%s', 'ts': %ld%ld }\n";
        
        const char* database = event_table->table_database.str;
        const char* table = event_table->table_name.str;
        struct timeval now;
        if (gettimeofday(&now, (void*)0) != 0) {
            return errno;
        }

        // 19 = max length of unsigned long long when interpreted as string
        size_t buf_size = (strlen(format) - 8) + strlen(database) + strlen(table) + 19 + 1;
        char buf[buf_size];
        memset(&buf, 0, buf_size);

        snprintf(buf, sizeof(buf), format, database, table, (long int)now.tv_sec, (long int)now.tv_usec);
        channel_put(connection, buf);
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
    char port[sizeof(unsigned int) * 8 + 1];
    memset(&port, 0, sizeof(port));
    sprintf(port, "%u", an_service_endpoint_port);
    connection = create_channel(an_service_endpoint_host, port);

    return 0;
}

static int async_notifications_deinit(void *arg MY_ATTRIBUTE((unused)))
{
    if (connection) {
        destroy_channel(connection);
        connection = 0;
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
    simple_status,                                         /* status variables                 */
    system_variables,                               /* system variables                 */
    NULL,                                           /* reserved for dependency checks   */
    0,                                              /* flags for plugin                 */
}
mysql_declare_plugin_end;

