//
// Copyright (c) 2014-2024 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_node_utils.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"

#define SM_NODE_LOCKED_FILE           "/var/persist/mtc/.node_locked"
#define SM_NODE_GO_ENABLE_FILE        "/var/run/goenabled"
#define SM_NODE_GO_ENABLE_FILE_SIMPLEX "/var/run/.goenabled"
#define SM_NODE_UNHEALTHY_FILE        "/var/run/.sm_node_unhealthy"
#define SM_NODE_CONFIG_COMPLETE_FILE  "/etc/platform/.initial_config_complete"
#define SM_NODE_PLATFORM_CONFIG_FILE  "/etc/platform/platform.conf"

static bool _failover_disabled = false;

static SmErrorT _sm_node_utils_get_system_mode_str( char system_mode[] );
static SmErrorT _sm_node_utils_get_system_type_str( char system_type[] );

typedef enum
{
    IsUnknown,
    IsTrue,
    IsFalse
}ThreeValuedT;

static ThreeValuedT _is_aio = IsUnknown;
static ThreeValuedT _is_aio_simplex = IsUnknown;
static ThreeValuedT _is_aio_duplex = IsUnknown;

// ****************************************************************************
// Node Utilities - Read Platform Config
// =====================================
static SmErrorT sm_node_utils_read_platform_config( const char key[],
    char value[], int value_size )
{
    FILE* fp;
    char format[1024];
    char line[1024];
    char val[1024];

    value[0] = '\0';

    fp = fopen( SM_NODE_PLATFORM_CONFIG_FILE, "r" );
    if( NULL == fp )
    {
        DPRINTFE( "Failed to open file (%s).", SM_NODE_PLATFORM_CONFIG_FILE );
        return( SM_FAILED );
    }

    snprintf( format, sizeof(format), "%s=%%%ds", (char*) key,
              (int) sizeof(val)-1 );

    while( NULL != fgets( line, sizeof(line), fp ) )
    {
        if( 1 == sscanf( line, format, val ) )
        {
            val[sizeof(val)-1] = '\0';
            snprintf( value, value_size, "%s", val );
            break;
        }
    }

    fclose( fp );

    if( '\0' == value[0] )
    {
        return( SM_NOT_FOUND );
    }
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get Node Type
// ==============================
static SmErrorT sm_node_utils_get_node_type( char node_type[] )
{
    return( sm_node_utils_read_platform_config( "nodetype", node_type,
                                                SM_NODE_TYPE_MAX_CHAR ) );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get Sub-Functions
// ==================================
static SmErrorT sm_node_utils_get_sub_functions( char sub_functions[] )
{
    return( sm_node_utils_read_platform_config( "subfunction", sub_functions,
                                                SM_NODE_SUB_FUNCTIONS_MAX_CHAR ) );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Node Type Is Controller
// ========================================
SmErrorT sm_node_utils_node_type_is_controller( bool* is_controller )
{
    char node_type[SM_NODE_TYPE_MAX_CHAR+1] = "";
    SmErrorT error;

    *is_controller = false;

    error = sm_node_utils_get_node_type( node_type );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get node type, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    if( 0 == strcmp( "controller", node_type ) )
    {
        *is_controller = true;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - system Is AIO
// ========================================
SmErrorT sm_node_utils_is_aio( bool* is_aio )
{
    if( IsUnknown == _is_aio )
    {
        char system_type[SM_SYSTEM_TYPE_MAX_CHAR];
        SmErrorT error;
        error = _sm_node_utils_get_system_type_str(system_type);
        if( SM_OKAY != error)
        {
            return error;
        }
        if( strcmp("All-in-one", system_type) == 0 )
        {
            _is_aio = IsTrue;
        }else
        {
            _is_aio = IsFalse;
        }
        *is_aio = ( IsTrue == _is_aio );
    }
    else
    {
        *is_aio = ( IsTrue == _is_aio );
    }

    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Sub-Functions Has Worker
// ==========================================
SmErrorT sm_node_utils_sub_function_has_worker( bool* has_worker )
{
    char sub_functions[SM_NODE_SUB_FUNCTIONS_MAX_CHAR+1] = "";
    SmErrorT error;

    *has_worker = false;

    error = sm_node_utils_get_sub_functions( sub_functions );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get sub-functions, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    if( NULL != strstr( sub_functions, "worker" ) )
    {
        *has_worker = true;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get Management Interface
// =========================================
SmErrorT sm_node_utils_get_mgmt_interface( char interface_name[] )
{
    return( sm_node_utils_read_platform_config( "management_interface",
                                                interface_name,
                                                SM_INTERFACE_NAME_MAX_CHAR ) );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get OAM Interface
// ==================================
SmErrorT sm_node_utils_get_oam_interface( char interface_name[] )
{
    return( sm_node_utils_read_platform_config( "oam_interface",
                                                interface_name,
                                                SM_INTERFACE_NAME_MAX_CHAR ) );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get Cluster Host Interface
// ===========================================
SmErrorT sm_node_utils_get_cluster_host_interface( char interface_name[] )
{
    return( sm_node_utils_read_platform_config( "cluster_host_interface",
                                                interface_name,
                                                SM_INTERFACE_NAME_MAX_CHAR ) );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get Admin Interface
// ===========================================
SmErrorT sm_node_utils_get_admin_interface( char interface_name[] )
{
    return( sm_node_utils_read_platform_config( "admin_interface",
                                                interface_name,
                                                SM_INTERFACE_NAME_MAX_CHAR ) );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get system mode string
// =============================================
static SmErrorT _sm_node_utils_get_system_mode_str( char system_mode[] )
{
    SmErrorT error = sm_node_utils_read_platform_config( "system_mode",
                                                system_mode,
                                                SM_SYSTEM_MODE_MAX_CHAR);

    if( SM_NOT_FOUND == error )
    {
        system_mode[0] = '\0';
        error = SM_OKAY;
    }
    return error;
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get system type string
// =============================================
static SmErrorT _sm_node_utils_get_system_type_str( char system_type[] )
{
    SmErrorT error = sm_node_utils_read_platform_config( "system_type",
                                                system_type,
                                                SM_SYSTEM_TYPE_MAX_CHAR);

    if( SM_NOT_FOUND == error )
    {
        system_type[0] = '\0';
        error = SM_OKAY;
    }
    return error;

}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get system mode string
// =============================================
SmSystemModeT sm_node_utils_get_system_mode()
{
    char system_mode[SM_SYSTEM_MODE_MAX_CHAR];
    char system_type[SM_SYSTEM_TYPE_MAX_CHAR];

    if ( SM_OKAY == _sm_node_utils_get_system_type_str(system_type) &&
         SM_OKAY == _sm_node_utils_get_system_mode_str(system_mode) )
    {
        if ( 0 == strcmp("All-in-one", system_type) )
        {
            if ( 0 == strcmp(SM_CPE_MODE_SIMPLEX, system_mode) )
            {
                return SM_SYSTEM_MODE_CPE_SIMPLEX;
            }
            else if ( 0 == strcmp(SM_CPE_MODE_DUPLEX, system_mode) )
            {
                return SM_SYSTEM_MODE_CPE_DUPLEX;
            }
            else if ( 0 == strcmp(SM_CPE_MODE_DUPLEX_DIRECT, system_mode) )
            {
                return SM_SYSTEM_MODE_CPE_DUPLEX_DC;
            }
        } else if ( 0 == strcmp("Standard", system_type) ) {
            return SM_SYSTEM_MODE_STANDARD;
        }
    }
    return SM_SYSTEM_MODE_UNKNOWN;
}

// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get Uptime
// ===========================
SmErrorT sm_node_utils_get_uptime( long* uptime )
{
    struct sysinfo info;
    int result;

    result = sysinfo( &info );
    if( 0 > result )
    {
        DPRINTFE( "Failed to get system uptime, error=%s.",
                  strerror( errno ) );
        return( SM_FAILED );
    }

    *uptime = info.uptime;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get Host Name
// ==============================
SmErrorT sm_node_utils_get_hostname( char node_name[] )
{
    char hostname[SM_NODE_NAME_MAX_CHAR];

    if( 0 > gethostname( hostname, sizeof(hostname) ) )
    {
        DPRINTFE( "Failed to get node name, error=%s.", strerror( errno ) );
        return( SM_FAILED );
    }

    snprintf( node_name, SM_NODE_NAME_MAX_CHAR, "%s", hostname );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Configuration Complete
// =======================================
SmErrorT sm_node_utils_config_complete( bool* complete )
{
    *complete = false;

    if( 0 > access( SM_NODE_CONFIG_COMPLETE_FILE, F_OK ) )
    {
        if( ENOENT == errno )
        {
            DPRINTFD( "Config-Complete file (%s) not available, error=%s.",
                      SM_NODE_CONFIG_COMPLETE_FILE, strerror( errno ) );
            return( SM_OKAY );

        } else {
            DPRINTFE( "Config-Complete file (%s) access failed, error=%s.",
                      SM_NODE_CONFIG_COMPLETE_FILE, strerror( errno ) );
            return( SM_FAILED );
        }
    }

    *complete = true;

    return( SM_OKAY );
}
// ****************************************************************************

typedef enum
{
    BLOCKING_STATE_INIT,
    WAIT_FOR_GOENABLED_FILE,
    WAIT_FOR_CONFIG_COMPLETE_FILE,
    NODE_UNHEALTHY_FILE_EXISTS,
    NODE_DISABLED_LICENSE_INVALID,
    NODE_DISABLED_FAILOVER,
    NODE_IS_LOCKED,
    NODE_ENABLED
}SmNodeEnabledBlockingStateT;
static SmNodeEnabledBlockingStateT blocking_state = BLOCKING_STATE_INIT;

// ****************************************************************************
// Node Utilities - Enabled
// ========================
// ****************************************************************************
SmErrorT sm_node_utils_enabled( bool* enabled, char reason_text[] )
{
    *enabled = false;
    reason_text[0] = '\0';
    const char* goenabled_file = SM_NODE_GO_ENABLE_FILE;
    const char* node_locked_file = SM_NODE_LOCKED_FILE;

    bool is_aio_simplex = false;
    SmErrorT error = sm_node_utils_is_aio_simplex(&is_aio_simplex);
    if(SM_OKAY != error)
    {
        DPRINTFE("Failed to get system mode, error %s",
            sm_error_str(error));
        return error;
    }

    if(is_aio_simplex)
    {
        goenabled_file = SM_NODE_GO_ENABLE_FILE_SIMPLEX;
    }

    // AIO SX Case: Need to support SM
    // 1. activating on the only locked controller
    // 2. maintaining an active state after locking the only controller
    if( (!is_aio_simplex) && ( 0 == access( node_locked_file, F_OK )))
    {
        if(blocking_state != NODE_IS_LOCKED)
        {
            blocking_state = NODE_IS_LOCKED;
            DPRINTFI("Node enable: blocked. Node is locked ; the %s file is present", node_locked_file);
        }

        snprintf( reason_text, SM_LOG_REASON_TEXT_MAX_CHAR,
                  "node is locked" );

        return( SM_OKAY );
    }

    if(0 > access( goenabled_file, F_OK ))
    {
        if( ENOENT == errno )
        {
            if(blocking_state != WAIT_FOR_GOENABLED_FILE)
            {
                blocking_state = WAIT_FOR_GOENABLED_FILE;
                DPRINTFI("Node enable: blocked. wait for goenabled file %s", goenabled_file);
            }

            snprintf( reason_text, SM_LOG_REASON_TEXT_MAX_CHAR,
                      "node not ready, go-enable not set" );

            return( SM_OKAY );

        } else {
            DPRINTFE( "Go-Enable file (%s) access failed, error=%s.",
                      SM_NODE_GO_ENABLE_FILE, strerror( errno ) );
            return( SM_FAILED );
        }
    }

    if( 0 > access( SM_NODE_CONFIG_COMPLETE_FILE, F_OK ) )
    {
        if( ENOENT == errno )
        {
            if(blocking_state != WAIT_FOR_CONFIG_COMPLETE_FILE)
            {
                blocking_state = WAIT_FOR_CONFIG_COMPLETE_FILE;
                DPRINTFI("Node enable: blocked. wait for config complete file %s", SM_NODE_CONFIG_COMPLETE_FILE);
            }

            snprintf( reason_text, SM_LOG_REASON_TEXT_MAX_CHAR,
                      "node not ready, config-complete not set" );

            return( SM_OKAY );

        } else {
            DPRINTFE( "Config-Complete file (%s) access failed, error=%s.",
                      SM_NODE_CONFIG_COMPLETE_FILE, strerror( errno ) );
            return( SM_FAILED );
        }
    }

    if( 0 == access( SM_NODE_UNHEALTHY_FILE, F_OK ) )
    {
        if(blocking_state != NODE_UNHEALTHY_FILE_EXISTS)
        {
            blocking_state = NODE_UNHEALTHY_FILE_EXISTS;
            DPRINTFI("Node enable: blocked. node unhealthy file %s found", SM_NODE_UNHEALTHY_FILE);
        }

        snprintf( reason_text, SM_LOG_REASON_TEXT_MAX_CHAR,
                  "node not ready, node unhealthy set" );

        return( SM_OKAY );

    } else {
        if( ENOENT != errno )
        {
            DPRINTFE( "Node unhealthy file (%s) access failed, error=%s.",
                      SM_NODE_UNHEALTHY_FILE, strerror( errno ) );
            return( SM_FAILED );
        }
    }

    if(_failover_disabled)
    {
        if(blocking_state != NODE_DISABLED_FAILOVER)
        {
            blocking_state = NODE_DISABLED_FAILOVER;
            DPRINTFI("Node enable: blocked. node has failed");
        }
        snprintf( reason_text, SM_LOG_REASON_TEXT_MAX_CHAR,
                  "Failover action to disable node" );
        return( SM_OKAY );
    }

    if(blocking_state != NODE_ENABLED)
    {
        blocking_state = NODE_ENABLED;
        DPRINTFI("Node enable: passed. node is enabled");
    }

    *enabled = true;
    snprintf( reason_text, SM_LOG_REASON_TEXT_MAX_CHAR, "node ready" );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Set Failover
// ==============================
bool sm_node_utils_set_failover( bool to_disable )
{
    bool old_value = _failover_disabled;
    if(to_disable)
    {
        DPRINTFI("disable system for failover");
    }
    _failover_disabled = to_disable;
    return old_value;
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Set Unhealthy
// ==============================
SmErrorT sm_node_utils_set_unhealthy( void )
{
    int fd = open( SM_NODE_UNHEALTHY_FILE,
                   O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH);
    if( 0 > fd )
    {
        DPRINTFE( "Failed to create file (%s), error=%s.",
                  SM_NODE_UNHEALTHY_FILE, strerror(errno) );
        return( SM_FAILED );
    }
    close(fd);
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - System Is AIO Simplex
// ======================================
SmErrorT sm_node_utils_is_aio_simplex( bool* is_aio_simplex )
{
    if( IsUnknown == _is_aio_simplex )
    {
        SmSystemModeT system_mode = sm_node_utils_get_system_mode();
        if( SM_SYSTEM_MODE_CPE_SIMPLEX == system_mode )
        {
            _is_aio_simplex = IsTrue;
        }else
        {
            _is_aio_simplex = IsFalse;
        }
    }

    *is_aio_simplex = ( IsTrue == _is_aio_simplex );
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Node Utilities - System is AIO Duplex
// =====================================
SmErrorT sm_node_utils_is_aio_duplex( bool* is_aio_duplex )
{
    if( IsUnknown == _is_aio_duplex )
    {
        SmErrorT error;
        bool is_aio = false;
        error = sm_node_utils_is_aio( &is_aio );
        if( SM_OKAY != error)
        {
            return error;
        }

        if ( !is_aio )
        {
            *is_aio_duplex = false;
            return SM_OKAY;
        }else
        {
            SmSystemModeT system_mode = sm_node_utils_get_system_mode();
            if(( SM_SYSTEM_MODE_CPE_DUPLEX == system_mode ) ||
	       ( SM_SYSTEM_MODE_CPE_DUPLEX_DC == system_mode ))
            {
                _is_aio_duplex = IsTrue;
            }else
            {
                _is_aio_duplex = IsFalse;
            }
        }
    }

    *is_aio_duplex = ( IsTrue == _is_aio_duplex );
    return SM_OKAY;
}

// ****************************************************************************
// Node Utilities - Clear the unhealthy flag
// ==============================
extern void sm_node_utils_reset_unhealthy_flag( void )
{
    if( 0 == access( SM_NODE_UNHEALTHY_FILE,  F_OK ) )
    {
        unlink( SM_NODE_UNHEALTHY_FILE );
        if( 0 == access( SM_NODE_UNHEALTHY_FILE,  F_OK ) )
        {
            DPRINTFE("file did not get removed ; %s", SM_NODE_UNHEALTHY_FILE);
        }
    }
}
