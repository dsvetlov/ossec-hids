/* @(#) $Id$ */

/* Copyright (C) 2003-2007 Daniel B. Cid <dcid@ossec.net>
 * All rights reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 3) as published by the FSF - Free Software
 * Foundation.
 *
 * License details at the LICENSE file included with OSSEC or 
 * online at: http://www.ossec.net/en/licensing.html
 */


#ifndef DBD
   #define DBD
#endif

#ifndef ARGV0
   #define ARGV0 "ossec-dbd"
#endif

#include "shared.h"
#include "dbd.h"



int main(int argc, char **argv)
{
    int c, test_config = 0;
    int uid = 0,gid = 0;

    /* Using MAILUSER (read only) */
    char *dir  = DEFAULTDIR;
    char *user = MAILUSER;
    char *group = GROUPGLOBAL;
    char *cfg = DEFAULTCPATH;


    /* Database Structure */
    DBConfig db_config;


    /* Setting the name */
    OS_SetName(ARGV0);
        

    while((c = getopt(argc, argv, "Vdhtu:g:D:c:")) != -1){
        switch(c){
            case 'V':
                print_version();
                break;
            case 'h':
                help();
                break;
            case 'd':
                nowDebug();
                break;
            case 'u':
                if(!optarg)
                    ErrorExit("%s: -u needs an argument",ARGV0);
                user=optarg;
                break;
            case 'g':
                if(!optarg)
                    ErrorExit("%s: -g needs an argument",ARGV0);
                group=optarg;
                break;
            case 'D':
                if(!optarg)
                    ErrorExit("%s: -D needs an argument",ARGV0);
                dir=optarg;
            case 'c':
                if(!optarg)
                    ErrorExit("%s: -c needs an argument",ARGV0);
                cfg = optarg;
                break;
            case 't':
                test_config = 1;    
                break;
            default:
                help();
                break;
        }

    }


    /* Starting daemon */
    debug1(STARTED_MSG, ARGV0);


    /* Check if the user/group given are valid */
    uid = Privsep_GetUser(user);
    gid = Privsep_GetGroup(group);
    if((uid < 0)||(gid < 0))
    {
        ErrorExit(USER_ERROR, ARGV0, user, group);
    }


    /* Reading configuration */
    if(OS_ReadDBConf(test_config, cfg, &db_config) < 0)
    {
        ErrorExit(CONFIG_ERROR, ARGV0, cfg);
    }


    /* Maybe disable this debug? */
    debug1("%s: DEBUG: Connecting to '%s', using '%s', '%s', '%s'.",
           ARGV0, db_config.host, db_config.user, 
           db_config.pass, db_config.db);
    
    
    /* Connecting to the database */
    db_config.conn = osdb_connect(db_config.host, db_config.user, 
                                  db_config.pass, db_config.db);
    if(!db_config.conn)
    {
        merror(DB_CONFIGERR, ARGV0);
        ErrorExit(CONFIG_ERROR, ARGV0, cfg);
    }

    
    /* Going on daemon mode */
    if(!test_config)
    {
        nowDaemon();
        goDaemon();
    }

    
    /* Privilege separation */	
    if(Privsep_SetGroup(gid) < 0)
        ErrorExit(SETGID_ERROR,ARGV0,group);

    
    /* chrooting */
    if(Privsep_Chroot(dir) < 0)
        ErrorExit(CHROOT_ERROR,ARGV0,dir);


    /* Now on chroot */
    nowChroot();


    /* Inserting server info into the db */
    db_config.server_id = OS_Server_ReadInsertDB(&db_config);
    if(db_config.server_id <= 0)
    {
        ErrorExit(CONFIG_ERROR, ARGV0, cfg);
    }


    /* Read rules and insert into the db */
    if(OS_InsertRulesDB(&db_config) < 0)
    {
        ErrorExit(CONFIG_ERROR, ARGV0, cfg);
    }

    
    /* Exit here if test config is set */
    if(test_config)
        exit(0);
        
        
    /* Changing user */        
    if(Privsep_SetUser(uid) < 0)
        ErrorExit(SETUID_ERROR,ARGV0,user);


    /* Basic start up completed. */
    debug1(PRIVSEP_MSG,ARGV0,dir,user);


    /* Signal manipulation */
    StartSIG(ARGV0);

    
    /* Creating PID files */
    if(CreatePID(ARGV0, getpid()) < 0)
        ErrorExit(PID_ERROR,ARGV0);

    
    /* Start up message */
    verbose(STARTUP_MSG, ARGV0, getpid());
    

    /* the real daemon now */	
    OS_DBD(&db_config);
    exit(0);
}


/* EOF */