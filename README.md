# MySQL async notifications
This mysql plugin notifies a connected endpoint of table access events.

## Supported mysql version
5.7.18

## Building
1. Download the mysql source from https://github.com/mysql/mysql-server.git
2. Copy the source directory async_notifications to the mysql-server/plugin/ directory
3. $ cd mysql-server; cmake
4. $ cd plugin/async_notifications; make
5. Copy the generated .so library to the mysql plugin directory (SHOW VARIABLES like 'plugin_dir')
6. Install the plugin using INSTALL PLUGIN async_notifications soname 'async_notifications.so'
7. Done

For more in-depth information, head over to https://dev.mysql.com/doc/refman/5.7/en/writing-plugins.html

## Usage
1. Set the following global system variables in your mysql instance:
```async_notifications_service_endpoint_host``` (default 127.0.0.1)
```async_notifications_service_endpoint_port``` (default 12345)

2. Start listening on the given host/port
3. Announce that you've registered:
```mysql> set @@global.async_notifications_service_endpoint_registered = 1;```
4. Done
