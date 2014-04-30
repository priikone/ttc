/*
 * options.h
 * 
 * Copyright (C) 1997, 1998 Pekka Riikonen, priikone@fenix.pspt.fi.
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

#ifndef _OPTIONS_H
#define _OPTIONS_H

#define MAXPARAMS 500

/* Structure for handling options */
typedef struct {
	char option;			/* option */
	char params[MAXPARAMS][256];	/* parameters */
	int parnum;			/* number of parameters */
} Option;

/* Options structure */
typedef struct {
	char *name;		/* option name */
	char *oletter;		/* character for option */
	int minpar;		/* min parameter required */
} Options;

/* All possible options */
static Options opts[] = {
        {"-d", 		"d", 1},
        {"deny",	"d", 1},
        {"Deny",	"d", 1},
        {"-t", 		"t", 1},
        {"timing", 	"t", 1},
        {"Timing", 	"t", 1},
        {NULL}
};

int read_options(char *filename, Option *option);
int find_parameters(const char *buffer, int len, Option *option);
void get_parameters(char *line, Option *option);
int checkline(char *buf);

#endif
