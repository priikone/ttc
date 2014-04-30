/*
 * common.c			Common functions for ttc
 * 
 * Copyright (c) 1999 Pekka Riikonen, priikone@poseidon.pspt.fi.
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
 * Revision 1.0  17.8.1999  pekka
 *	Added to ttc
 */

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>

/* Performs wildcard search for tty string and compares the two tty
strings. Returns 0 if strings match, otherwise -1. Available wildcards
are ? and *. This function is only used when options are read from
file. getopt() parses automatically options when given from command
line (a lot slower, though). */

int check_tty(char *string1, char *string2, int fromfile)
{
	int i;
	static char tmpstring[256];
	static char tmpstr[256];
	int slen1 = strlen(string1);
	int slen2 = strlen(string2);

	strncpy(tmpstr, string2, slen2);

  if (fromfile) {

	/* see if they are same already */
	if (!strcmp(string1, tmpstr))
	    return 0;

	if (slen2 < slen1)
	    if (!strchr(string1, '*'))
	    	return -1;

	strncpy(tmpstring, string2, slen2);

	for (i = 0; i < slen2; i++) {

	    /* * wildcard. Only one * wildcard is possible. */
	    if (string1[i] == '*')
		if (!strncmp(string1, tmpstr, i)) {
		    bzero(tmpstr, slen2);
		    strncpy(tmpstr, string1, i);
		    break;
		}

	    /* ? wildcard */
	    if (string1[i] == '?') {
		if (!strncmp(string1, tmpstr, i)) {
		    if (!(slen1 < i + 1))
			if (string1[i + 1] != '?' &&
			    string1[i + 1] != tmpstr[i + 1])
			    continue;

		    if (!(slen1 < slen2))
		    	tmpstr[i] = '?';
		}
	    } else {
		if (strncmp(string1, tmpstr, i))
		    strncpy(tmpstr, tmpstring, slen2);
	    }
	}

	/* if using *, remove it */
	if (strchr(string1, '*'))
	    *strchr(string1, '*') = 0;

	bzero(tmpstring, sizeof(tmpstring));

  } /* fromfile */
	if (!strcmp(string1, tmpstr)) {
	    bzero(tmpstr, sizeof(tmpstr));
	    return 0;
	}

	bzero(tmpstr, sizeof(tmpstr));
	return -1;
}

/* Get's users logname. If for some reason environment variable LOGNAME or
getlogin() return no username, we'll take it from /etc/passwd file to be
sure. */

char *getname()
{
        char *logname;

        if ((logname = getenv("LOGNAME")) == (char *)NULL)
            if ((logname = getlogin()) == (char *)NULL) {
		struct passwd *pw;

        	if ((pw = getpwuid(getuid())) == 0) {
            	    fprintf(stderr, "ttc: getname(): %s\n", strerror(errno));
            	    exit(1);
        	}

		logname = pw->pw_name;
	    }

        return logname;
}
