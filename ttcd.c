/* 
 * ttcd.c			Terminal Type Control Daemon.
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
 * Revision 1.42  15.3.1999  pekka
 *      Some bug fixes and code improvements
 *      fixed sigchld(), should leave zombies no more
 *
 * Revision 1.41  9.3.1998  pekka
 *	Some minor code fix
 *	Fixed -c option bug in clean_files()
 *
 * Revision 1.40  26.5.1997  pekka
 *	Fixed recvfrom()'s for other systems in receive_messages()
 *	Fixed some typos
 *	Added max PID (SID) number for flood test in timing()
 *	Added logins count per day for users -feature
 *
 * Revision 1.39  15.2.1997  pekka
 *	'First' release version.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <string.h>
#include "config.h"		/* configuration file */

void sigchld();
void receive_messages();
void timing(char *logname, char *terminal, int timelimit,
	int notimebank, int sid, int loglimit);
void clean_files();
void create_tlfile(int dmins, char *logname, 
		int logspd, int notimebank);
void printo(char *message, char *tty);
void log(char *mesg, char *lname);


int main(argc, argv)
int argc;
char *argv[];
{
	char opt;			/* option */

	if (argc < 2) {			/* if no options */
		if (getuid() != 0) {
		    printf("ttcd: Sorry, only root may run ttcd.\n");
		    exit(1); 
		}
		if(fork())		/* fork child to be dameon */
		   exit(0);		/* parent exits */

		log("ttc daemon started", "root");
		setsid();

		receive_messages();	/* child (main daemon)*/
	} else {
	    while ((opt = getopt(argc, argv, "chV")) != EOF) /* get options */
		switch(opt) {
		case 'c':
		 clean_files();
		 exit(0);
		case 'V':
 printf("Terminal Type Control Daemon, version 1.42 (C) 15.3.1999 Pekka Riikonen\n");
		 exit(0);
		case 'h':
		default:
		 printf("usage: ttcd [-c] [-hV]\n");
		 printf("    -c    clean time limit files\n");      
		 printf("    -h    display this message\n");      
		 printf("    -V    display version\n");      
		 exit(0);
	    }
	}

	exit(0);
}	

/* Called when SIGCHLD is received. This way we prevent child zombies. */

void sigchld(int sig)
{
	int stat;

#ifdef TEST_DEBUG
	log("SIGCHLD received", "TEST_DEBUG");
#endif
	wait(&stat);

#ifdef TEST_DEBUG
	if (WIFEXITED(stat))
	    log("Normal child exit", "TEST_DEBUG");
#endif
	signal(SIGCHLD, sigchld);
}

/* This function receives messages from ttc-socket. It stops the process 
until some data is received from socket. After that it forks a new child 
which then handles the timing. Then, it remains waiting new messages. */

void receive_messages()
{
	static char cmesg[2][12];	/* [0] = username
					 * [1] = current tty */
	int sock;			/* socket */
	int pid;			/* child's pid */
	static int imesg[4];		/* [0] = time limit
					 * [1] = notimebank
					 * [2] = PPID
					 * [3] = login limit */

	static char fname[15];		/* filename for socket */
	struct sockaddr_un addr;	/* socket address */
	int i;

	/* received info */
	char *terminal;
	char *logname;
	int timelimit;
	int notimebank;
	int sid;
	int loginlimit;

	bzero(cmesg, sizeof(cmesg));
	bzero(imesg, sizeof(imesg));

	sprintf(fname, "/var/lock/ttcd");/* socket path */
	unlink(fname);			/* remove old socket */

	/* create socket */
	if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
	    log("Couldn't create socket.", "socket()");
	    exit(1);
	}

#ifdef TEST_DEBUG
	log("Socket created ok", "TEST_DEBUG");
#endif

	addr.sun_family = AF_UNIX;	/* protocol family */
	strcpy(addr.sun_path, fname);

	/* bind socket - create socket file */
	if ((bind(sock, (struct sockaddr *)&addr, sizeof(addr))) == -1) {
	    log("Couldn't bind the socket.", "bind()");
	    exit(1);
	} 

	/* correct permissions */
	chown(fname, getuid(), getgid());
	chmod(fname, 0766);

	signal(SIGCHLD, sigchld);

	/* let's loop forever and be ready to receive messages */
	for (;;) {

	    for (i = 0; i < 2; i++) {
 		recvfrom(sock, &cmesg[i], 12, 0, (struct sockaddr *)&addr,
			(int *)sizeof(addr));
	    }
	    for (i = 0; i < 4; i++) {
 		recvfrom(sock, &imesg[i], 6, 0, (struct sockaddr *)&addr,
			(int *)sizeof(addr));
	    }	

	    if ((pid = fork()) == 0)		/* fork child */
		break;
	    if (pid == -1)
	    	log("fork(): Couldn't fork timing()", (char *)cmesg[0]);
	    else
	  	log("Timing started", (char *)cmesg[0]);
	}					/* parent continues here */

	signal(SIGCHLD, SIG_DFL);		/* default action to SIGCHLD */
	setsid();				/* create new session */

	/* get the received info */
	logname = cmesg[0];
	terminal = cmesg[1];
	timelimit = imesg[0];
	notimebank = imesg[1];
	sid = imesg[2];
	loginlimit = imesg[3];

	/* call child */
	timing(logname, terminal, timelimit, notimebank, sid, loginlimit);
}

/* Child and timing routine for ttc. This is the routine which is forked 
as child process by ttcd. This handles the timing for ttc. */

void timing(char *logname, char *terminal, int timelimit,
	int notimebank, int sid, int loglimit)
{
	time_t curtime;
	struct tm *tptr;
	static char filename[18];
	
	int dtmp;
	int logspd;
	int hour, 	/* hours */
	min, 		/* minutes */
	cmins, 		/* current time formed to minutes */
	dmins, 		/* destination time formed to minutes */
	signal;		/* signal from kill() */

	int a_time = A_TIME,
	a_minutes = A_MINUTES,
	k_seconds = K_SECONDS,
	s_seconds = A_MINUTES * 60; 	/* form A_MINUTES to seconds */

	logspd = loglimit;
	if (logspd > 0)
	    logspd -= 1;		/* minus 1 login */

	curtime = time(0);		/* get time */
 	tptr = localtime(&curtime);
	hour = tptr->tm_hour;
	min = tptr->tm_min;

	bzero(filename, sizeof(filename));
	strcpy(filename, "/tmp/ttc/");
	strcat(filename, logname);	/* filename */

	cmins = (hour * 60) + min;	/* current minutes */
	if (!notimebank || (notimebank && logspd >= 0) ||
	    (notimebank && timelimit >= 0))
	    a_time = timelimit;
	dmins = cmins + a_time; 	/* destination minutes */

	/* this way we prevent some flood messages perhaps */
	if (a_time > A_TIME || a_time < -1) {
	    log("Bad destination time error", logname);
	    exit(1);
	}

	if (notimebank < 0 || notimebank > 1) {
	    log("Bad check status error", logname);
	    exit(1);
	}

	if (sid < 0 || sid > 32767) {		/* max 32767 ? */
	    log("Bad pid number error", logname);
	    exit(1);
	}

	if (logspd < -1 || logspd > 10000) {
	    log("Bad logins per day number error", logname);
	    exit(1);
	}

	/* create the time limit file if time bank is used. */
	if (!notimebank || (notimebank && logspd >= 0)) {
	    /* time remaining for today */
	    dtmp = dmins - cmins;
	    create_tlfile(dtmp, logname, logspd, notimebank); 
	}

	/* if a_time is -1 then login limit is used
	but user still has unlimited access. */
	if (a_time == -1)
            exit(0);

	/* loop until time is up */
	while(cmins < dmins) {

	    /* current time + a_minutes */
	    cmins = cmins + a_minutes;

	    if (!notimebank) {
		/* time remaining for today */
		dtmp = dmins - cmins;
		create_tlfile(dtmp, logname, logspd, notimebank); 
	    }

	    /* if process is dead, user has logged out */
	    signal = kill(sid, SIGCONT);
	    if (signal == -1) {
		if (!notimebank) {
	 	    /* time remaining for today */
		    dmins = dmins - cmins;
		    create_tlfile(dmins, logname, logspd, notimebank);
		}

		log("Timing stopped", logname);
		exit(0);
	    }

	    /* if current time is 23:59 (in minutes 1439) */
	    if (cmins > 1439) {

		/* start new time limit */
		printo(N_MESSAGE, terminal);
		sleep(60);		/* sleep for 60 seconds */
		cmins = 0;		/* current mins to zero */
		dmins = A_TIME; 	/* a new time limit */

		log("Day changed. New time limit starting", logname);
	        if (!notimebank)
		    create_tlfile(dmins, logname, logspd, notimebank);
		continue;
	    }

	    /* sleep before next round */
	    sleep(s_seconds);
	}

	signal = kill(sid, SIGCONT);
	if (signal != -1) {
	    printo(MESSAGE, terminal);
	    sleep(k_seconds); 		/* wait before killing */
	}
	log("Time limit reached", logname); 

	/* kill SID == shell's PID */
	kill(sid, SIGKILL);
	exit(0);
}

/* Clear's a day old time limit files */

void clean_files()
{
	if(getuid() != 0) {
	    printf("ttcd: Sorry, only root may use this option.\n");
	    exit(1);
	}

	system("rm -rf /tmp/ttc");
	mkdir("/tmp/ttc", 0755);
	log("Time limit files removed", "root");
}

/* Creates time limit file for user. Saves also login limits for
the user if defined. */

void create_tlfile(int dmins, char *logname, int logspd, int notimebank)
{
	FILE *cn;
	static char filename[18];
	int b, k;
	
	strcpy(filename, "/tmp/ttc/");
	strcat(filename, logname);	/* filename */

	if ((cn = fopen(filename, "w")) == 0)
	    log("Couldn't create time limit file", logname);

	for (k = 0; k < logspd; k++)
	    fputc(26, cn);

	if (!notimebank) {
	    if (dmins < 0)
		fputc(24, cn);

	    for (b = 0; b < dmins; b++)
	    	fputc(27, cn);
	} else
	    fputc(25, cn);

	fclose(cn);
	bzero(filename, sizeof(filename));
}

/* Prints message to specific tty */

void printo(char *message, char *tty)
{
	FILE *fd;

	/* open device */
	if ((fd = fopen(tty, "w")) == NULL)
	    log("Couldn't open tty for writing", "fopen()");

	fprintf(fd, "%s\007\n", message);
	fclose(fd);
}

/* Makes log messages into ttc logfile if DEBUG is defined. */

void log(char *mesg, char *lname)
{
#ifdef DEBUG			/* logs only if DEBUG defined */
	time_t cutime;
	char *ctime();
	FILE *fp;
	char *logfile = "/var/log/ttcdlog";	/* logfile */

	cutime = time(0);			/* current time */

	if ((fp = fopen(logfile, "a+")) == NULL)
		printf("ttcd: %s.\n", strerror(errno));

	fprintf(fp, "ttcd: %s by %s %s", (char *)mesg, lname, ctime(&cutime));
	fclose(fp);
#endif
}
