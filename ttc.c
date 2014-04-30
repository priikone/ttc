/* 
 * ttc.c			Terminal Type Control
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
 * Revision 3.1  17.8.1999  pekka
 *	Minor code changes
 *	Relocated couple functions to common.c
 *
 * Revision 3.0  23.4.1999  pekka
 *	Some code fixes
 *
 * Revision 2.8  15.3.1999  pekka
 *	Some bug fixes
 *	Time limit is now shown also on the very first logon
 *
 * Revision 2.7  9.3.1998  pekka
 *	Improved option handling
 *	Added new sort of configuration handling for ttc
 *	  ttc.config -file, see code for more
 *	Removed -l option (not needed anymore)
 *	Removed -s option (not needed anymore)
 *	Added new deny_access() function
 *	Added usage of wildcards in names of tty's
 *	Improved some code
 *	Fixed some minor bugs
 *
 * Revision 2.6  28.5.1997  pekka
 *	Fixed sendto() for other systems in send_message()
 *	Added -l option for login count checks per day
 *	Added ^C signal to be set default before exiting
 *
 * Revision 2.5  24.2.1997  pekka
 *	Removed suid-root
 *	Made ttcd.c, ttc daemon to handle timing
 *	ttc now uses sockets to communicate between ttc and ttcd
 *	 so we don't need to run it as suid-root
 *	Made options.c for better option handling
 * 
 * Revision 2.2  7.2.1997  pekka
 *	added -static flag in compiling for suid-root
 *
 * Revision 2.1  5.2.1997  pekka
 *	Added timing routine into ttc.c
 *	ttc runs now as suid-root
 *
 * Revision 1.7  26.1.1997  pekka
 *	Made all new timing system	
 *
 * Revision 1.0  23.1.1997  pekka
 *	First release version
 */

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pwd.h>

#include "config.h"
#include "options.h"
#include "confauth.h"
#include "common.h"

void do_options(Option *optr);
void usage();
void deny_tty(char *ttyd);
void deny_access(char *terminal, Config *config, int rn);
void time_tty(char *ttyt);
void send_message(int a_time, int loginlimit, int check, 
		char *logname, char *ttype);
void check_timing(int *time, int *lt, char *logname);

int fromfile = 1;
int optnum;
int confnum;
Option option[10];
Config config[700];

int main(argc, argv)
int argc;
char *argv[];
{
	int ret;
	int c = 1;			/* counter for options */
	char opt;			/* option */
	static char *filename;
	static char *ttype;
	static char *logname;

	ttype = ttyname(0);		/* get current tty */
	logname = getname();		/* get logname */

	filename = DEFOPTION;

	signal(SIGINT, SIG_IGN);	/* ignore ^C */

	if (argc < 2) {
	    /* read options from file */
	    optnum = read_options(filename, option);

	    /* read configuration file */
	    confnum = read_conf(CONF_FILE, config);

	    /* do DenyAccess check for the user */
	    if ((ret = do_confauth(logname, config, 
			DENY_ACCESS, confnum)) >= 0)
	    	deny_access(ttype, config, ret);

	    /* do options */
	    do_options(option);
	} else {
	    /* get options from command line */
	    fromfile = 0;

	    /* read configuration file */
	    confnum = read_conf(CONF_FILE, config);

	    /* do DenyAccess check for the user */
	    if ((ret = do_confauth(logname, config, 
			DENY_ACCESS, confnum)) >= 0)
	    	deny_access(ttype, config, ret);

	    while ((opt = getopt(argc, argv, "Vdtfh")) != EOF)
		switch(opt) {
		case 'V':
printf("Terminal Type Control, version 3.1 (C) 20.8.1999 Pekka Riikonen\n");
		    exit(0);
		    break;
		repeat:
		case 'd':
		    for (c = c + 1; c < argc; c++) {
 			/* check if there is a match */ 
			if (!strcmp(argv[c], "-t"))
			    break;

			deny_tty(argv[c]); 
		    }
		    break;
		case 't':
		    for (c = c + 1; c < argc; c++) {
 			/* check if there is a match */
			if (!strcmp(argv[c], "-d"))
			    goto repeat;

			time_tty(argv[c]);
		    }
		    break;
		case 'f':
		    /* use different option file */
		    filename = argv[c + 1];

	    	    /* read options from file */
	    	    optnum = read_options(filename, option);

	    	    /* do options */
	    	    do_options(option);
		    break;
		case 'h':
		default:
		    usage();
	    }
	}

	signal(SIGINT, SIG_DFL);	/* default action for ^C */
	exit(0);
}	

/* Parses options from options file for ttc */

void do_options(Option *optr)
{
	int i;
	int y;
	int c;

	fromfile = 1;
	for (i = 0; i < optnum; i++) {
	    switch(optr[i].option) {
	    case 'd':
		c = 0;
	    	for (y = 0; y < optr[i].parnum; y++) {
	    	    deny_tty(optr[i].params[c]); 
		    c++;
		}
	        break;
	    case 't':
		c = 0;
	    	for (y = 0; y < optr[i].parnum; y++) {
	    	    time_tty(optr[i].params[c]); 
		    c++;
		}
		break;
	    case 'V':
	    	printf("You can't use -V option in options file");
		exit(1);
	    case 'f':
	    	printf("You can't use -f option in options file");
		exit(1);
	    default:
		usage();
	    }
	}
}

void usage()
{
	printf("usage: ttc [-dt [/dev/tty ...] -f [options file] -hV]\n");
	printf("   -d <ttylist>   deny tty's (e.g. -d /dev/tty1 /dev/tty2 ...)\n");
	printf("   -t <ttylist>   timing for tty's (e.g. -t /dev/ttyS1 ...)\n");
	printf("   -f <file>      read options from file (default: %s)\n", DEFOPTION);      
	printf("   -h             display this message\n");
	printf("   -V             display version\n");
	exit(1);
}

/* Do the deny function. This checks if user has right to access denied 
tty. If not user will be killed. */

void deny_tty(char *ttyd)
{
	static char *ttype;
	static char *logname;
	int pid, sid;

	ttype = ttyname(0);		/* get current tty */
	logname = getname();		/* get logname */

	/* 
	 * If current tty matches with one from parameters,
	 * authentication is done. If authentication fails
	 * users's session will be killed.
	 */
	if(!check_tty(ttyd, ttype, fromfile))

	    /* do authentication */
	    if (do_confauth(logname, config, DENY_AUTH, confnum) >= 0) {
		printf(A_MESSAGE"\n");
		exit(0);			/* passed */
	    } else {

	    	/* authentication failed */
	    	fprintf(stderr, D_MESSAGE"\007\n");
		fflush(stderr);
	    	pid = getpid();
	    	sid = getsid(pid);
	    	kill(sid, 9);			/* kill this session */
		exit(1);
	    }
}

/* This function is called when user's name was found in DenyAccess
section. This means that user might not be allowed to logon on current
terminal. This checks for that. */

void deny_access(char *terminal, Config *config, int rn)
{
	int i, c, len;
	int pid, sid;
	int tokenpos[256], count = 0;
	static char tmptty[20];

	/* if no options, access is denied
	to all terminals. */
	if (!config[rn].options) {
	    fprintf(stderr, D_MESSAGE"\007\n");
	    fflush(stderr);
	    pid = getpid();
	    sid = getsid(pid);
	    kill(sid, SIGKILL);
	    exit(1);
	}

	len = strlen(config[rn].options);

	for (i = 0; i <= len; i++)
	    if (config[rn].options[i] == ' ' ||
		config[rn].options[i] == '\0')
		tokenpos[count++] = i;

	if (!fromfile)
	    fromfile = 2;

	c = 0;
	for (i = 0; i < count; i++) {
	    strncpy(tmptty, config[rn].options + c, (tokenpos[i] - c));
	    tmptty[tokenpos[i]] = '\0';
	    c = tokenpos[i] + 1;

	    /* check the tty for a match */
	    if (!check_tty(tmptty, terminal, fromfile)) {
	    	fprintf(stderr, D_MESSAGE"\007\n");
		fflush(stderr);
	    	pid = getpid();
	    	sid = getsid(pid);
	    	kill(sid, SIGKILL);
		exit(1);
	    }

	    memset(tmptty, 0, sizeof(tmptty));
	}

	if (fromfile == 2)
	    fromfile = 0;
}

/* Do the time check function. Checks if user has access to timed tty 
without timing. If not, message will be sent to ttcd to start timing for 
user. */

void time_tty(char *ttyt)
{
	int a_time = -1, notimebank = 0, loginlimit = -1;
	int ret, i, j, len;
	static char *ttype;
	static char *logname;
	static char timelimit[10];
	static char ntimeb[15];
	static char loglimit[10];

	memset(timelimit, 0, sizeof(timelimit));
	memset(ntimeb, 0, sizeof(ntimeb));
	memset(loglimit, 0, sizeof(loglimit));

	ttype = ttyname(0);		/* get current tty */
	logname = getname();		/* get logname */

	/* 
	 * If current tty matches with one from parameters,
	 * authentication is done. If authentication fails
	 * user will have time limit.
	 */
	if(!check_tty(ttyt, ttype, fromfile)) {

	    /* do authentication */
	    ret = do_confauth(logname, config, TIMING_AUTH, confnum);

	    /* no name found from config file,
	    start normal timing procedure. */
	    if (ret < 0) {
		a_time = A_TIME;

		/* check the remaining time */
		check_timing(&a_time, &loginlimit, logname);
	    }

	    /* name found in config file */
	    if (ret >= 0) {

		/* if no options, no time limit,
		unlimited access granted. */
		if (!config[ret].options) {
		    printf(A_MESSAGE"\n");
		    fflush(stdout);
		    exit(0);
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
		check_timing(&a_time, &loginlimit, logname);

		if (!a_time && loginlimit >= 0)
		    a_time = -1;

		/* if a_time is -1, user has unlimited access
		and login limit. */
		if (a_time == -1) {
		    printf(A_MESSAGE"\n");
		    fflush(stdout);
		}
	    }

	    /* send message to socket */
	    send_message(a_time, loginlimit, notimebank, logname, ttype);
	}
}

/* Sends message to ttc-socket what ttcd listens for incoming messages. */

void send_message(int time, int loglimit, int ntb, 
		char *logname, char *ttype)
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
		exit(1); 
	    }
	}

	for (i = 0; i < 4; i++) {
 	    if(sendto(sock, &imsg[i], 6, 0, (struct sockaddr *)&addr,
			sizeof(addr)) == -1) {
		fprintf(stderr, "sendto: %s\n", strerror(errno));
		exit(1);
	    }
	}
}

/* Checks out users time limit, if any. This also checks the login counts
remaining for today, if any. */

void check_timing(int *time, int *lt, char *logname)
{
	int pid, sid, check;
	int loginlimit, atime;
	static char filename[18] = "/tmp/ttc/";
	FILE *tl;

	pid = getpid();
	sid = getsid(pid);

	strcat(filename, logname);	/* filename */

	/* open time limit file */
	if ((tl = fopen(filename, "r")) == NULL) {
	    printf(TIMELEFT"\n", *time);
	    fflush(stdout);
	    return;
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
		    kill(sid, SIGKILL);
		    exit(1);
		}

		/* illegal file type */
		fprintf(stderr, FMESSAGE"\007\n");
		fflush(stderr);
		kill(sid, SIGKILL);
		exit(1); 
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
		kill(sid, SIGKILL);
		exit(1);
	    }
	}
	fclose(tl);

	if (*lt != -1) {
	    /* if login limit is up, kill session */
	    if (loginlimit < 1) {
		printf(LLMESSAGE"\007\n");
		fflush(stdout);
		kill(sid, SIGKILL);
		exit(1); 
	    }

	    /* logons remaining */
	    *lt = loginlimit;
	}

	/* print the time left */
	if (atime > 0)
	    printf(TIMELEFT"\n", atime);

	fflush(stdout);

	/* time remaining */
	*time = atime;

	/* print the login limits left */
	if (*lt != -1) {
	    printf(LOGLEFT"\n", loginlimit - 1);
	    fflush(stdout);
	}
}
