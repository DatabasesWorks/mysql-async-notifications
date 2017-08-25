#!/usr/bin/env bash

MYSQL=`which mysql`
MYSQL_DEFAULTS_FILE=$1
MYSQL_PLUGIN=$2

NO_MYSQL=1
NO_DEFAULTS_FILE=2
NO_MYSQL_PLUGIN=3
SHOW_USAGE=4
NO_PLUGIN_DIR=5
UNINSTALL_FAILED=6
NOT_ROOT=7

function ensure_root {
    if [[ $(whoami) != "root" ]] ; then
        return $NOT_ROOT
    fi

    return 0
}

function process_and_verify_args {
    if [[ -z $MYSQL_DEFAULTS_FILE ]] ; then
        echo "No arguments provided"
        return $SHOW_USAGE
    elif [[ ! -f $MYSQL ]] ; then
        return $NO_MYSQL
    elif [[ $MYSQL_DEFAULTS_FILE = "--help" ]] ; then
        return $SHOW_USAGE
    elif [[ $MYSQL_DEFAULTS_FILE = "-h" ]] ; then
        return $SHOW_USAGE
    elif [[ ! -f $MYSQL_DEFAULTS_FILE ]] ; then
        return $NO_DEFAULTS_FILE
    elif [[ ! -f $MYSQL_PLUGIN ]] ; then
        return $NO_MYSQL_PLUGIN
    fi

    MYSQL_DEFAULTS_FILE=$(realpath $MYSQL_DEFAULTS_FILE)

    return 0
}

function show_usage {

    echo "Usage: $0 [--help | -h]"
    echo "Usage: $0 <defaults file> <compiled plugin>"
    echo ""
    echo "defaults file:      A MySQL defaults file, containing username and password to connect."
    echo "compiled plugin:    The compiled plugin to install"
}

function get_plugin_dir {
    sql="SHOW VARIABLES LIKE 'plugin_dir';"
    dir=$($MYSQL --defaults-file=$MYSQL_DEFAULTS_FILE -e "$sql" -s -r | tail -n 1 | awk '{$1=""; print $0}')
    echo $dir
}

function copy_plugin {
    plugin_dir=$(get_plugin_dir)
    if [[ ! -d $plugin_dir ]] ; then
        return $NO_PLUGIN_DIR
    fi

    cp $MYSQL_PLUGIN $plugin_dir
    return $?
}

function uninstall_existing_plugin {
    plugin_soname=$(basename $MYSQL_PLUGIN)
    plugin_name="${plugin_soname%.*}"
    show_plugins_sql="SHOW PLUGINS;"
    uninstall_plugin_sql="UNINSTALL PLUGIN $plugin_name;"
    installed=`$MYSQL --defaults-file=$MYSQL_DEFAULTS_FILE -e "$show_plugins_sql" -s -r | grep -i $plugin_name | wc -l`

    if [[ $installed -ne 0 ]] ; then
        echo "Uninstalling pre-existing plugin"
        $MYSQL --defaults-file=$MYSQL_DEFAULTS_FILE -e "$uninstall_plugin_sql"
        uninstall_state=$?

        if [[ $uninstall_state -ne 0 ]] ; then
            return $UNINSTALL_FAILED
        fi
    fi

    return 0
}

function install_plugin {
    plugin_soname=$(basename $MYSQL_PLUGIN)
    plugin_name="${plugin_soname%.*}"
    install_plugin_sql="INSTALL PLUGIN $plugin_name SONAME '$plugin_soname';"

    $MYSQL --defaults-file=$MYSQL_DEFAULTS_FILE -e "$install_plugin_sql"
    return $?
}

function show_default_settings {
    plugin_soname=$(basename $MYSQL_PLUGIN)
    plugin_name="${plugin_soname%.*}"

    plugin_settings_sql="SHOW VARIABLES LIKE '%$plugin_name%';"

    $MYSQL --defaults-file=$MYSQL_DEFAULTS_FILE -e "$plugin_settings_sql"
    return $?
}

function do_plugin_install {
    uninstall_existing_plugin
    uninstall_existing_plugin_result=$?
    if [[ $uninstall_existing_plugin_result -ne 0 ]] ; then
        return $uninstall_existing_plugin_result
    fi

    echo "Copying plugin to installation directory"
    copy_plugin
    copy_plugin_result=$?
    if [[ $copy_plugin_result -ne 0 ]] ; then
        return $copy_plugin_result
    fi

    echo "Installing plugin"
    install_plugin
    install_plugin_result=$?
    if [[ $install_plugin_result -ne 0 ]] ; then
        return $install_plugin_result
    fi

    echo "Plugin installed, default settings are:"
    show_default_settings

    return 0            
}

process_and_verify_args
process_and_verify_args_result=$?

if [[ $process_and_verify_args_result -eq $NO_MYSQL ]] ; then
    (>&2 echo "MySQL commandline client not found")
    exit 1
elif [[ $process_andverify_args_result -eq $NO_DEFAULTS_FILE ]] ; then
    (>&2 echo "MySQL defaults file '$MYSQL_DEFAULTS_FILE' does not exist")
    exit 1
elif [[ $process_and_verify_args_result -eq $NO_MYSQL_PLUGIN ]] ; then
    (>&2 echo "Plugin '$MYSQL_PLUGIN' does not exist")
    exit 1
elif [[ $process_and_verify_args_result -eq $SHOW_USAGE ]] ; then
    show_usage
    exit 0
fi

ensure_root
ensure_root_result=$?

if [[ $ensure_root_result -eq $NOT_ROOT ]] ; then
    (>&2 echo "Must be root to do this")
    exit 1
fi

do_plugin_install
do_plugin_install_result=$?

if [[ $do_plugin_install_result -eq $NO_PLUGIN_DIR ]] ; then
    (>&2 echo "Could not determine mysql plugin directory")
    exit 1
elif [[ $do_plugin_install_result -eq $UNINSTALL_FAILED ]] ; then
    (>&2 echo "Could not uninstall pre-existing plugin")
    exit 1
elif [[ ! $do_plugin_install_result ]] ; then
    (>&2 echo "Error when installing plugin")
    exit 1
fi

echo "Done."
exit 0

