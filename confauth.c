/*
 * confauth.c			Ttc configuration functions,
 *				configuration file functions,
 *				authentication functions
 * 
 * Copyright (C) 1998 - 1999 Pekka Riikonen, priikone@poseidon.pspt.fi.
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
 * Revision 1.2  17.8.1999  pekka
 *	very minor code changes
*
 * Revision 1.1  15.3.1999  pekka
 *	Fixed ngets() and read_conf()
 *
 * Revision 1.0  9.3.1998  pekka
 *	Added to ttc
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "confauth.h"

/* Reads ttc configuration file and parses it. Returns number of
configuration lines. */

int read_conf(char *filename, Config *config)
{
	int fd, begin;
	char *buffer, *cp;
	char line[1024];
	ConfSection *cptr;
	int i, filelen, type, maxfield = 0;

	/* open configuration file */
	if ((fd = open(filename, O_RDONLY)) < 0)
	    return -1;

        /* inspect file length */
        filelen = lseek(fd, (off_t)0L, SEEK_END);
        lseek(fd, (off_t)0L, SEEK_SET);

	/* allocate memory for input buffer */
	buffer = (char *)malloc(sizeof(char) * filelen + 1);

        /* read file into buffer */
        if ((read(fd, buffer, filelen)) == -1) {
            memset(buffer, 0, sizeof(buffer));
            close(fd);
            return -1;
        }
        close(fd);

	/* read() won't take EOF, but we'll need it. */
	buffer[filelen] = EOF;

	/* 
	 * parse configuration file
	 */
	i = 0;
	type = TTC_NONE;
	begin = 0;
	while((begin = ngets(line, sizeof(line), buffer, filelen, begin)) != EOF) {
	    cp = line;

	    /* check for bad line */
	    if (check_line(cp))
		continue;

	    /* parse line */
	    switch(cp[0]) {
		/* start of new section */
		case '<':
	    	    /* remove new line sign */
	    	    if (strchr(cp, '\n'))
			*strchr(cp, '\n') = '\0';

		    type = TTC_NONE;
		    for (cptr = section; cptr->section; cptr++)
	    		if (!strcmp(cp, cptr->section)) {
			    type = cptr->type;
			    maxfield = cptr->maxfields;
			}
		break;
		/* start of option line */
		case '+':
		    if (type != TTC_NONE) {

		    	/* add section type */
			config[i].type = type;

		    	/* parse configuration line */
		    	if (parse_conf(cp, &config[i], maxfield) < 0) {
			    cp[strlen(cp) - 1] = '\0';
			    fprintf(stderr,
		"ttc: Incorrect configuration line: %s (Ignored).\n", cp);
			    break;
			}

		    	i++;
		    }
		break;
		default:		/* bad line */
		break;
	    }
	}
	free(buffer);

	return i;
}

/* Gets line from buffer. Stops reading when newline or EOF occurs.
This doesn't remove the newline sign from the destination buffer. */

int ngets(char *dest, int destlen, const char *src, int srclen, int begin)
{
	static int start = 0;
	int i;

	memset(dest, 0, destlen);

	if (begin != start)
	    start = 0;

	i = 0;
	for (start; start <= srclen; i++, start++) {
	    if (i > destlen)
		return -1;		/* error */

	    dest[i] = src[start];	/* put char */

	    if (dest[i] == EOF) 
		return EOF;

	    if (dest[i] == '\n') 
		break;
	}
	start++;		/* add 1 for next char */

	return start;
}

/* Parses sections configuration lines. Gets username and other possible
options. */

int parse_conf(char *buf, Config *config, int maxfield)
{
	int i, len, check;
	char *cp, *dp;
	int tokenpos[10];

	cp = &buf[1];			/* exclude '+' character */
	dp = &buf[1];

	memset(&tokenpos, 0, sizeof(tokenpos));

	if (strchr(cp, '\n'))
	    *strchr(cp, '\n') = ':';

	/* find out position of field marks in string */
	check = 1;
	if (strchr(dp, ':'))
	    for (i = 0; i < maxfield; i++) {
		if (strchr(dp, ':'))
		    check++;

	    	tokenpos[i] = strcspn(dp, ":");
		dp = dp + tokenpos[i] + 1;
	    }

	/* check that line is correct */
	if ((check - 1) != maxfield) {
	    return -1;
	}

	config->username = (char *)NULL;
	config->options = (char *)NULL;

	/* 
	 * get the user name (will be NULL terminated)
	 */
	if (tokenpos[0]) {
	    config->username = (char *)malloc(sizeof(char) * (size_t)tokenpos[0] + 1);
	    memcpy(config->username, cp, (size_t)tokenpos[0]);

	    config->username[tokenpos[0]] = '\0';
	    cp = cp + tokenpos[0] + 1;
	} else
	    return -1;

	/* 
	 * get the options if any. Options will
	 * be parsed later for specific use.
	 */
	if (!tokenpos[1] && !tokenpos[2] && !tokenpos[3])
	    return 0;

	/* get the length of remaining line */
	len = 0;
	for (i = 1; i < maxfield; i++)
	    len += tokenpos[i] + 1;

	/* allocate memory for options */
	config->options = (char *)malloc(sizeof(char) * (size_t)len + 1);
	dp = config->options;

	/* copy the option string and add termination sign */
	memcpy(dp, cp, (size_t)len - 1);
	dp[len - 1] = '\0';

	return 0;
}

/* Performs authentication for user. Depending on the authentication
type if username is matched in authentication, user will get access
to denied tty or will get no time limit on timed tty's. When using
DenyAccess section this will check if the name was found or not in
the section.

Returns:

-1 		authentication failed, name not found or error
record number	authentication passed

*/

int do_confauth(const char *logname, Config *config, 
	int type, int confnum)
{
	int i;

	switch(type) {
	    case TIMING_AUTH:
		for (i = 0; i < confnum; i++)
		    if (config[i].type == TIMING_AUTH) {
			if (!strcmp(config[i].username, "ALL"))
			    return i;
		    	if (!strcmp(logname, config[i].username))
			    return i;
		    }
	    break;
	    case DENY_AUTH:
		for (i = 0; i < confnum; i++)
		    if (config[i].type == DENY_AUTH) {
			if (!strcmp(config[i].username, "ALL"))
			    return i;
			if (!strcmp(logname, config[i].username))
			    return i;
		    }
	    break;
	    case DENY_ACCESS:
		for (i = 0; i < confnum; i++)
		    if (config[i].type == DENY_ACCESS) {
			if (!strcmp(logname, config[i].username))
			    return i;
		    }
	    break;
	    default:
		fprintf(stderr, "do_confauth: Illegal authentication type.");
		return -1;
	}

	return -1;
}


/* Checks line for illegal characters. Returns -1 when illegal
character found. This accepts only lines starting with <, which
is section start character and + which is configuration start
character. */

int check_line(char *buf)
{

	/* illegal characters in line */
        if (strchr(buf, '#')) return -1;
        if (strchr(buf, '@')) return -1;
        if (strchr(buf, '&')) return -1;
        if (strchr(buf, '$')) return -1;
        if (strchr(buf, '%')) return -1;
        if (strchr(buf, '\'')) return -1;
        if (strchr(buf, '\\')) return -1;
        if (strchr(buf, '\t')) return -1;
        if (strchr(buf, '\r')) return -1;
        if (strchr(buf, '\a')) return -1;
        if (strchr(buf, '\b')) return -1;
        if (strchr(buf, '\f')) return -1;
        if (strchr(buf, '"')) return -1;

	/* empty line */
	if (buf[0] == '\n')
	    return -1;

	/* illegal configuration line */
	if (buf[0] != '<' && buf[0] != '+')
	    return -1;

	/* line is ok */
        return 0;
}
