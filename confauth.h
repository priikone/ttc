/*
 * confauth.h
 * 
 * Copyright (C) 1998 Pekka Riikonen, priikone@fenix.pspt.fi.
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

#ifndef _CONFAUTH_H
#define _CONFAUTH_H

/* config sections */
#define TTC_NONE	0	/* NULL */
#define TIMING_AUTH 	1	/* timing authentication */
#define DENY_AUTH 	2	/* deny authentication */
#define DENY_ACCESS 	3	/* deny access section */

/* Config structure */
typedef struct {
	unsigned int type;	/* config section type */
	char *username;		/* user name */
	char *options;		/* possible options */
} Config;

/* Config section structure */
typedef struct {
	char *section;		/* section name */
	int maxfields;		/* max fields */
	int type;		/* section type */
} ConfSection;

/* all sections */
static ConfSection section[] = {
        {"<TimingAuth>",	4, TIMING_AUTH},
        {"<DenyAuth>",		1, DENY_AUTH},
        {"<DenyAccess>",	2, DENY_ACCESS},
        {NULL}
};

int read_conf(char *filename, Config *config);
int ngets(char *dest, int destlen, const char *src, int srclen, int begin);
int parse_conf(char *buf, Config *config, int maxfield);
int do_confauth(const char *logname, Config *config, int type, int confnum);
int check_line(char *buf);

#endif
