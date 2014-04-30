/*
 * options.c		Option handling functions for ttc.
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
 * Revision 1.4  17.8.1999  pekka
 *	very minor code changes
 *
 * Revision 1.3  15.3.1999  pekka
 *	Some bug fixes
 *
 * Revision 1.2  9.3.1998  pekka
 *	Added Option structure for easier option handling
 *	Major code change for better
 *	Fixed some bugs
 *
 * Revision 1.1  26.5.1997  pekka
 *	Added -l option
 *	Added login count options
 *
 * Revision 1.0  21.2.1997  pekka
 *	First released version
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
#include "options.h"
#include "confauth.h"


/* Reads options from options file and return's number of options. */

int read_options(char *filename, Option *option)
{	
	int fd, begin;
	int a = 0, h = 0;
	int filelen;
	Options *optp;
	static char *buffer;
	static char line[1024];
	static char tmp[256];
	char *cp;

        /* open options file */
        if ((fd = open(filename, O_RDONLY)) < 0)
            return -1;

        /* inspect file length */
        filelen = lseek(fd, (off_t)0L, SEEK_END);
        lseek(fd, (off_t)0L, SEEK_SET);

	if (filelen < 5)
	    return -1;

        /* allocate memory for input buffer */
        buffer = (char *)malloc(sizeof(char) * filelen + 1);
	memset(buffer, 0, filelen);
	memset(line, 0, sizeof(line));

        /* read file into buffer */
        if ((read(fd, buffer, filelen)) == -1) {
            memset(buffer, 0, filelen);
            close(fd);
            return -1;
        }
        close(fd);
 
	/* read() won't take EOF, but we'll need it. */
        buffer[filelen] = EOF;
 
	/* find out number of parameters in options */
	find_parameters(buffer, filelen, option);

	begin = 0;
	while ((begin = ngets(line, sizeof(line), buffer, filelen, begin)) != EOF) {
	    cp = line;

	    if (checkline(cp))
		continue;

            for (a = 0; a < strlen(cp); a++)
		if (cp[a] == ' ' || cp[a] == '\n')
		    break;

	    strncpy(tmp, cp, a);
	    tmp[a + 1] = '\0';

	    /* is there a match */
	    for (optp = opts; optp->name; optp++)
	    	if (!strcmp(tmp, optp->name)) {

		    if (option[h].parnum < optp->minpar) {
			printf("ttc: Too few parameters in %s option.\n",
				tmp);
			exit(1);
		    }

		    option[h].option = *optp->oletter;

		    if (option[h].parnum != 0)
		    	/* get parameters for option */
		    	get_parameters(cp + a + 1, &option[h]);

		}
		h++;
		memset(tmp, 0, sizeof(tmp));
	}

	return h;			/* return number of options */
}

/* find out number of parameters in options. */

int find_parameters(const char *buffer, int len, Option *option)
{
	static char line[1024];
	char *cp;
	int i, linelen, begin;
	int parameters = 0, a = 0;

	memset(line, 0, sizeof(line));
	begin = 0;
	while ((begin = ngets(line, sizeof(line), buffer, len, begin)) != EOF) {
	    cp = line;

	    if (checkline(cp))
		continue;

	    linelen = strlen(cp);
	    for (i = 0; i < linelen; i++)
		if (cp[i] == ' ')
		    parameters++;

	    option[a++].parnum = parameters;
     	    parameters = 0;
	}

	return a;

}

/* parses the line in words, seperated by whitespace. This is used
to get parameters from options in option line. */

void get_parameters(char *line, Option *option)
{
	int i, j;
	int k = 0, b = 0;
	static char tmpbuf[256];

	for (i = 0; i < strlen(line); i++) {
	    if (line[i] != ' ' && line[i] != '\n')
		tmpbuf[k++] = line[i];
	    else {
		tmpbuf[k] = '\0';
		k = 0;
		for (j = 0; j < strlen(tmpbuf); j++)
		    option->params[b][j] = tmpbuf[j];
		b++;
		memset(tmpbuf, 0, sizeof(tmpbuf));
		continue;
	    }
	}

	memset(tmpbuf, 0, sizeof(tmpbuf));
}

/* checks the line for illegal characters. Returns -1 if
line is illegal. */

int checkline(char *buf)
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

        /* line is ok */
        return 0;
}                                                   
