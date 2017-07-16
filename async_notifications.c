#include <stdio.h>
#include <mysql/plugin.h>
#include <mysql/plugin_audit.h>
#include <string.h>
#include <channel.h>

static channel connection;

static int async_notifications_notify(MYSQL_THD thd, 
        mysql_event_class_t event_class,
        const void *event)
{
    if (event_class == MYSQL_AUDIT_TABLE_ACCESS_CLASS) {
        const struct mysql_event_table_access *event_table = 
            (const struct mysql_event_table_access *)event;

        // todo fprintf
        const char* format = "Access event on table [%s].[%s]\n";
        const char* database = event_table->table_database.str;
        const char* table = event_table->table_name.str;
        size_t buf_size = (strlen(format) - 4) + strlen(database) + strlen(table) + 1;
        char buf[buf_size];
        memset(&buf, 0, buf_size);

        snprintf(buf, buf_size, format, database, table);
        async_put(connection, buf);
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
    connection = create_channel("127.0.0.1", "12345");
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

static struct st_mysql_show_var status[] =
{
    { "Async_Notifications_Status",
        "OK",
        SHOW_CHAR_PTR, SHOW_SCOPE_GLOBAL
    },
    { 0, 0, 0, SHOW_SCOPE_GLOBAL }
};

static struct st_mysql_sys_var* system_variables[] = 
{
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
    status,                                         /* status variables                 */
    system_variables,                               /* system variables                 */
    NULL,                                           /* reserved for dependency checks   */
    0,                                              /* flags for plugin                 */
}
mysql_declare_plugin_end;

