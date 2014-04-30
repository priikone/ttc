/* 
 * pam_ttc.c			Terminal Type Control PAM Module
 * 
 * Copyright (C) 1997 - 1999 Pekka Riikonen, priikone@poseidon.pspt.fi.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * Revision 1.0  25.3.1999  pekka
 *	Added to ttc
 */

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pwd.h>
#include <syslog.h>
#include <security/pam_modules.h>
#include <security/pam_appl.h>

#include "config.h"
#include "options.h"
#include "confauth.h"
#include "common.h"

int pam_ttc(pam_handle_t *pamh, int flags, int argc, const char **argv);
int do_options(Option *optr);
int deny_tty(char *ttyd);
int deny_access(char *terminal, Config *config, int rn);
int time_tty(char *ttyt);
int send_message(int a_time, int loginlimit, int check, 
		const char *logname, char *ttype);
int check_timing(int *time, int *lt, const char *logname);

int optnum;
int confnum;
Option option[10];
Config config[700];
char *ttype;
const char *logname;

/*
 * Performs ttc authentication. Called on account management.
 */
int pam_ttc(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	int ret;
	char *filename;

	/* get current tty */
	ttype = ttyname(0);

	/* get logname */
	if ((ret = pam_get_item(pamh, PAM_USER, (void *)&logname))
		!= PAM_SUCCESS)
	    return ret;

	filename = DEFOPTION;

	/* read options from file */
	optnum = read_options(filename, option);

	/* read configuration file */
	confnum = read_conf(CONF_FILE, config);

	/* do DenyAccess check for the user */
	if ((ret = do_confauth(logname, config, 
		DENY_ACCESS, confnum)) >= 0)
	    if ((ret = deny_access(ttype, config, ret)) 
		!= PAM_SUCCESS)
		return ret;

	/* do options */
	if ((ret = do_options(option)) != PAM_SUCCESS)
	    return ret;

	return PAM_SUCCESS;
}

/* Parses options from options file for ttc */

int do_options(Option *optr)
{
	int i, y, c, ret;

	for (i = 0; i < optnum; i++) {
	    switch(optr[i].option) {
	    case 'd':
		c = 0;
	    	for (y = 0; y < optr[i].parnum; y++) {
	    	    ret = deny_tty(optr[i].params[c]);
		    if (ret != PAM_IGNORE)
			return ret;
		    c++;
		}
	        break;
	    case 't':
		c = 0;
	    	for (y = 0; y < optr[i].parnum; y++) {
	    	    ret = time_tty(optr[i].params[c]); 
		    if (ret != PAM_IGNORE)
			return ret;
		    c++;
		}
		break;
	    case 'V':
		syslog(LOG_AUTHPRIV|LOG_ERR, 
		"pam_ttc: You can't use -V option in options file");
		return PAM_AUTH_ERR;
	    case 'f':
		syslog(LOG_AUTHPRIV|LOG_ERR, 
		"pam_ttc: You can't use -f option in options file");
		return PAM_AUTH_ERR;
	    default:
		syslog(LOG_AUTHPRIV|LOG_ERR, 
		"pam_ttc: Illegal option '%s' in options file",
			optr[i].option);
		return PAM_AUTH_ERR;
	    }
	}

	return PAM_SUCCESS;
}

/* Do the deny function. This checks if user has right to access denied 
tty. */

int deny_tty(char *ttyd)
{

	/* 
	 * If current tty matches with one from parameters,
	 * authentication is done. If authentication fails
	 * users's access will be denied.
	 */
	if(!check_tty(ttyd, ttype, 1)) {

	    /* do authentication */
	    if (do_confauth(logname, config, DENY_AUTH, confnum) >= 0) {
		fprintf(stdout, A_MESSAGE"\n");
		fflush(stdout);
		syslog(LOG_AUTHPRIV|LOG_INFO, 
			"pam_ttc: Unlimited access granted for %s to %s",
			logname, ttype);
		return PAM_SUCCESS;		/* passed */
	    } else {

	    	/* authentication failed */
	    	fprintf(stderr, D_MESSAGE"\007\n");
		fflush(stderr);
		syslog(LOG_AUTHPRIV|LOG_INFO, 
		"pam_ttc: Access denied for %s to %s", logname, ttype);
		return PAM_PERM_DENIED;
	    }
	}
	
	return PAM_IGNORE;
}

/* This function is called when user's name was found in DenyAccess
section. This means that user might not be allowed to logon on current
terminal. This checks for that. */

int deny_access(char *terminal, Config *config, int rn)
{
	int i, c, len;
	int tokenpos[256], count = 0;
	char tmptty[20];

	/* if no options, access is denied
	to all terminals. */
	if (!config[rn].options) {
	    fprintf(stderr, D_MESSAGE"\007\n");
	    fflush(stderr);
	    syslog(LOG_AUTHPRIV|LOG_INFO, 
		"pam_ttc: Access denied for %s to %s",
		logname, terminal);
	    return PAM_PERM_DENIED;
	}

	len = strlen(config[rn].options);

	for (i = 0; i <= len; i++)
	    if (config[rn].options[i] == ' ' ||
		config[rn].options[i] == '\0')
		tokenpos[count++] = i;

	c = 0;
	for (i = 0; i < count; i++) {
	    strncpy(tmptty, config[rn].options + c, (tokenpos[i] - c));
	    tmptty[tokenpos[i]] = '\0';
	    c = tokenpos[i] + 1;

	    /* check the tty for a match */
	    if (!check_tty(tmptty, terminal, 1)) {
	    	fprintf(stderr, D_MESSAGE"\007\n");
		fflush(stderr);
	    	syslog(LOG_AUTHPRIV|LOG_INFO, 
			"pam_ttc: Access denied for %s to %s",
			logname, terminal);
		return PAM_PERM_DENIED;
	    }

	    memset(tmptty, 0, sizeof(tmptty));
	}

	return PAM_SUCCESS;
}

/* Do the time check function. Checks if user has access to timed tty 
without timing. If not, message will be sent to ttcd to start timing for 
user. */

int time_tty(char *ttyt)
{
	int a_time = -1, notimebank = 0, loginlimit = -1;
	int ret, ret2, i, j, len;
	char timelimit[10];
	char ntimeb[15];
	char loglimit[10];

	memset(timelimit, 0, sizeof(timelimit));
	memset(ntimeb, 0, sizeof(ntimeb));
	memset(loglimit, 0, sizeof(loglimit));

	/* 
	 * If current tty matches with one from parameters,
	 * authentication is done. If authentication fails
	 * user will have time limit.
	 */
	if(!check_tty(ttyt, ttype, 1)) {

	    /* do authentication */
	    ret = do_confauth(logname, config, TIMING_AUTH, confnum);

	    /* no name found from config file,
	    start normal timing procedure. */
	    if (ret < 0) {
		a_time = A_TIME;

		/* check the remaining time */
		ret2 = check_timing(&a_time, &loginlimit, logname);
		if (ret2 == PAM_PERM_DENIED || ret2 == PAM_AUTH_ERR)
		    return ret2;
	    }

	    /* name found in config file */
	    if (ret >= 0) {

		/* if no options, no time limit,
		unlimited access granted. */
		if (!config[ret].options) {
		    fprintf(stdout, A_MESSAGE"\n");
		    fflush(stdout);
		    syslog(LOG_AUTHPRIV|LOG_INFO, 
			"pam_ttc: Unlimited access granted for %s to %s",
			logname, ttype);
		    return PAM_SUCCESS;
		}

		/*
		 * parse config line options
		 */
		len = strlen(config[ret].options);

        	/* get time limit */
		i = 0;
        	j = 0; 
        	if (strchr(config[ret].options, ':')) {
            	    for (i = 0; i < len; i++) {
                	if (config[ret].options[i] != ':' &&
                    	    config[ret].options[i] != '\0') {
                    	    timelimit[j++] = config[ret].options[i]; 
                	} else 
                    	    break; 
            	    }
            	    timelimit[j] = '\0'; 
		    a_time = atoi(timelimit);
        	}

        	/* get notimebank option */ 
        	j = 0; 
        	if (strchr(config[ret].options, ':')) {
	            for (i = i + 1; i < len; i++) {
            	    	if (config[ret].options[i] != ':' &&
                	    config[ret].options[i] != '\0') {
                	    ntimeb[j++] = toupper(config[ret].options[i]); 
            	    	} else
                	    break;
        	    }
        	    ntimeb[j] = '\0'; 
		}

        	/* get login limit option */ 
        	j = 0; 
        	if (strchr(config[ret].options, ':')) {
	            for (i = i + 1; i < len; i++) {
            	    	if (config[ret].options[i] != ':' &&
                	    config[ret].options[i] != '\0') {
                	    loglimit[j++] = config[ret].options[i];
            	    	} else
                	    break;
        	    }
        	    loglimit[j] = '\0'; 
		    if (j)
		        loginlimit = atoi(loglimit);
		}

		/* see if notimebank option was found */
		if (!strncmp(ntimeb, "NOTIMEBANK", 10)) {
		    notimebank = 1; 
		    if (!a_time)
			a_time = A_TIME;
	    	}

		/* check the remaining time and login
		limits for the user. */
		ret2 = check_timing(&a_time, &loginlimit, logname);
		if (ret2 == PAM_PERM_DENIED || ret2 == PAM_AUTH_ERR)
		    return ret2;

		if (!a_time && loginlimit >= 0)
		    a_time = -1;

		/* if a_time is -1, user has unlimited access
		and login limit. */
		if (a_time == -1) {
		    fprintf(stdout, A_MESSAGE"\n");
		    fflush(stdout);
		}
	    }

	    /* send message to socket */
	    ret2 = send_message(a_time, loginlimit, notimebank, 
		logname, ttype);
	    return ret2;
	}
	
	return PAM_IGNORE;
}

/* Sends message to ttc-socket what ttcd listens for incoming messages. */

int send_message(int time, int loglimit, int ntb, 
		const char *logname, char *ttype)
{
	int pid = getsid(getpid());

	/* messages we send to ttc daemon socket */
	char *cmsg[2] =
			{ (char *)logname,	/* username */ 
			  (char *)ttype 	/* current tty */
			};
	int imsg[4] =  
			{ time,			/* time limit */
			  ntb,			/* notimebank */
			  pid,			/* SID == PPID */
			  loglimit 		/* login limit */
			};

	struct sockaddr_un addr;
	int sock;				/* socket */
	char fname[15];				/* filename for socket */
	int i;

	sprintf(fname, SOCKET_PATH"ttcd");	/* socket path */
	
	sock = socket(AF_UNIX, SOCK_DGRAM, 0);	/* create socket */
	addr.sun_family = AF_UNIX;		/* UNIX socket */
	strcpy(addr.sun_path, fname);

	/* send messages to daemon socket */
	for (i = 0; i < 2; i++) {
 	    if(sendto(sock, cmsg[i], 12, 0, (struct sockaddr *)&addr, 
			sizeof(addr)) == -1) {
		fprintf(stderr, "sendto: %s\n", strerror(errno));
		syslog(LOG_AUTHPRIV|LOG_ERR, 
			"pam_ttc: sendto: %s", strerror(errno));
		return PAM_AUTH_ERR;
	    }
	}

	for (i = 0; i < 4; i++) {
 	    if(sendto(sock, &imsg[i], 6, 0, (struct sockaddr *)&addr,
			sizeof(addr)) == -1) {
		fprintf(stderr, "sendto: %s\n", strerror(errno));
		syslog(LOG_AUTHPRIV|LOG_ERR, 
			"pam_ttc: sendto: %s", strerror(errno));
		return PAM_AUTH_ERR;
	    }
	}

	return PAM_SUCCESS;
}

/* Checks out users time limit, if any. This also checks the login counts
remaining for today, if any. */

int check_timing(int *time, int *lt, const char *logname)
{
	int pid, sid, check;
	int loginlimit, atime;
	char filename[18] = "/tmp/ttc/";
	FILE *tl;

	pid = getpid();
	sid = getsid(pid);

	strcat(filename, logname);	/* filename */

	/* open time limit file */
	if ((tl = fopen(filename, "r")) == NULL) {
	    fprintf(stdout, TIMELEFT"\n", *time);
	    fflush(stdout);
	    return PAM_SUCCESS;
	}

	atime = 0;
	loginlimit = 0;

	/* count the login limit */
	while((check = fgetc(tl)) != 27) {
	    if (check == 26)
		loginlimit++;
	    else {
		/* this is found when time bank system
		is not used but login limits are saved
		in the same file. */
		if (check == 25) {
		    atime = *time;
		    break;
		}

		/* this is found when login limit is
		used but user still has unlimited access,
		i.e. no time limit. */
		if (check == 24) {
		    atime = -1;
		    break;
		}

		/* if at the end of file already,
		it is zero file and time is up. */
		if (check == EOF) {
		    fprintf(stderr, TLMESSAGE"\007\n");
		    fflush(stderr);
	    	    syslog(LOG_AUTHPRIV|LOG_ERR, 
			"pam_ttc: Time limit is up for %s on %s",
			logname, ttype);
		    return PAM_PERM_DENIED;
		}

		/* illegal file type */
		fprintf(stderr, FMESSAGE"\007\n");
		fflush(stderr);
	    	syslog(LOG_AUTHPRIV|LOG_ERR, 
			"pam_ttc: Illegal time file type for %s on %s",
			logname, ttype);
		return PAM_AUTH_ERR;
	    }
	}
	if (check == 27)
	    atime = 1;

	/* count the time limit */
	while((check = fgetc(tl)) != EOF) {
	    if (check == 27)
		atime++;
	    else {
		/* illegal file type */
		fprintf(stderr, FMESSAGE"\007\n");
		fflush(stderr);
	    	syslog(LOG_AUTHPRIV|LOG_ERR, 
			"pam_ttc: Illegal time file type for %s on %s",
			logname, ttype);
		return PAM_AUTH_ERR;
	    }
	}
	fclose(tl);

	if (*lt != -1) {
	    /* if login limit is up, kill session */
	    if (loginlimit < 1) {
		fprintf(stdout, LLMESSAGE"\007\n");
		fflush(stdout);
	    	syslog(LOG_AUTHPRIV|LOG_INFO, 
			"pam_ttc: Login limit is up for %s on %s",
			logname, ttype);
		return PAM_PERM_DENIED;
	    }

	    /* logons remaining */
	    *lt = loginlimit;
	}

	/* print the time left */
	if (atime > 0)
	    fprintf(stdout, TIMELEFT"\n", atime);

	fflush(stdout);

	/* time remaining */
	*time = atime;

	/* print the login limits left */
	if (*lt != -1) {
	    fprintf(stdout, LOGLEFT"\n", loginlimit - 1);
	    fflush(stdout);
	}

	return PAM_SUCCESS;
}

/*
 * Account management side of the module.
 */
PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, 
			int argc, const char **argv)
{
	return pam_ttc(pamh, flags, argc, argv);
}

/*
 * All the other PAM service calls will be ignored.
 */
PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, 
			int argc, const char **argv)
{
	return PAM_IGNORE;
}
PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, 
			int argc, const char **argv)
{
	return PAM_IGNORE;
}
PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, 
			int argc, const char **argv)
{
	return PAM_IGNORE;
}
PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, 
			int argc, const char **argv)
{
	return PAM_IGNORE;
}
PAM_EXTERN int pam_sm_chauthok(pam_handle_t *pamh, int flags, 
			int argc, const char **argv)
{
	return PAM_IGNORE;
}
