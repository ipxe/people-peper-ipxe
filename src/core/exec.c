/*
 * Copyright (C) 2006 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <assert.h>
#include <ipxe/tables.h>
#include <ipxe/command.h>
#include <ipxe/parseopt.h>
#include <ipxe/settings.h>
#include <ipxe/shell.h>

/** @file
 *
 * Command execution
 *
 */

/** Shell stop state */
static int stop_state;

/**
 * Execute command
 *
 * @v command		Command name
 * @v argv		Argument list
 * @ret rc		Return status code
 *
 * Execute the named command.  Unlike a traditional POSIX execv(),
 * this function returns the exit status of the command.
 */
int execv ( const char *command, char * const argv[] ) {
	struct command *cmd;
	int argc;

	/* Count number of arguments */
	for ( argc = 0 ; argv[argc] ; argc++ ) {}

	/* An empty command is deemed to do nothing, successfully */
	if ( command == NULL )
		return 0;

	/* Sanity checks */
	if ( argc == 0 ) {
		DBG ( "%s: empty argument list\n", command );
		return -EINVAL;
	}

	/* Reset getopt() library ready for use by the command.  This
	 * is an artefact of the POSIX getopt() API within the context
	 * of Etherboot; see the documentation for reset_getopt() for
	 * details.
	 */
	reset_getopt();

	/* Hand off to command implementation */
	for_each_table_entry ( cmd, COMMANDS ) {
		if ( strcmp ( command, cmd->name ) == 0 )
			return cmd->exec ( argc, ( char ** ) argv );
	}

	printf ( "%s: command not found\n", command );
	return -ENOEXEC;
}

/**
 * Split command line into tokens
 *
 * @v command		Command line
 * @v tokens		Token list to populate, or NULL
 * @ret count		Number of tokens
 *
 * Splits the command line into whitespace-delimited tokens.  If @c
 * tokens is non-NULL, any whitespace in the command line will be
 * replaced with NULs.
 */
static int split_command ( char *command, char **tokens ) {
	int count = 0;

	while ( 1 ) {
		/* Skip over any whitespace / convert to NUL */
		while ( isspace ( *command ) ) {
			if ( tokens )
				*command = '\0';
			command++;
		}
		/* Check for end of line */
		if ( ! *command )
			break;
		/* We have found the start of the next argument */
		if ( tokens )
			tokens[count] = command;
		count++;
		/* Skip to start of next whitespace, if any */
		while ( *command && ! isspace ( *command ) ) {
			command++;
		}
	}
	return count;
}

/**
 * Process next command only if previous command succeeded
 *
 * @v rc		Status of previous command
 * @ret process		Process next command
 */
static int process_on_success ( int rc ) {
	return ( rc == 0 );
}

/**
 * Process next command only if previous command failed
 *
 * @v rc		Status of previous command
 * @ret process		Process next command
 */
static int process_on_failure ( int rc ) {
	return ( rc != 0 );
}

/**
 * Find command terminator
 *
 * @v tokens		Token list
 * @ret process_next	"Should next command be processed?" function
 * @ret argc		Argument count
 */
static int command_terminator ( char **tokens,
				int ( **process_next ) ( int rc ) ) {
	unsigned int i;

	/* Find first terminating token */
	for ( i = 0 ; tokens[i] ; i++ ) {
		if ( tokens[i][0] == '#' ) {
			/* Start of a comment */
			break;
		} else if ( strcmp ( tokens[i], "||" ) == 0 ) {
			/* Short-circuit logical OR */
			*process_next = process_on_failure;
			return i;
		} else if ( strcmp ( tokens[i], "&&" ) == 0 ) {
			/* Short-circuit logical AND */
			*process_next = process_on_success;
			return i;
		}
	}

	/* End of token list */
	*process_next = NULL;
	return i;
}

/**
 * Set shell stop state
 *
 * @v stop		Shell stop state
 */
void shell_stop ( int stop ) {
	stop_state = stop;
}

/**
 * Test and consume shell stop state
 *
 * @v stop		Shell stop state to consume
 * @v stopped		Shell had been stopped
 */
int shell_stopped ( int stop ) {
	int stopped;

	/* Test to see if we need to stop */
	stopped = ( stop_state >= stop );

	/* Consume stop state */
	if ( stop_state <= stop )
		stop_state = 0;

	return stopped;
}

/**
 * Execute command line
 *
 * @v command		Command line
 * @ret rc		Return status code
 *
 * Execute the named command and arguments.
 */
int system ( const char *command ) {
	int ( * process_next ) ( int rc );
	char *expcmd;
	char **argv;
	int argc;
	int count;
	int process;
	int rc = 0;

	/* Perform variable expansion */
	expcmd = expand_settings ( command );
	if ( ! expcmd )
		return -ENOMEM;

	/* Count tokens */
	count = split_command ( expcmd, NULL );

	/* Create token array */
	if ( count ) {
		char * tokens[count + 1];
		
		split_command ( expcmd, tokens );
		tokens[count] = NULL;
		process = 1;

		for ( argv = tokens ; ; argv += ( argc + 1 ) ) {

			/* Find command terminator */
			argc = command_terminator ( argv, &process_next );

			/* Execute command */
			if ( process ) {
				argv[argc] = NULL;
				rc = execv ( argv[0], argv );
			}

			/* Stop processing, if applicable */
			if ( shell_stopped ( SHELL_STOP_COMMAND ) )
				break;

			/* Stop processing if we have reached the end
			 * of the command.
			 */
			if ( ! process_next )
				break;

			/* Determine whether or not to process next command */
			process = process_next ( rc );
		}
	}

	/* Free expanded command */
	free ( expcmd );

	return rc;
}

/**
 * Concatenate arguments
 *
 * @v args		Argument list (NULL-terminated)
 * @ret string		Concatenated arguments
 *
 * The returned string is allocated with malloc().  The caller is
 * responsible for eventually free()ing this string.
 */
char * concat_args ( char **args ) {
	char **arg;
	size_t len;
	char *string;
	char *ptr;

	/* Calculate total string length */
	len = 1 /* NUL */;
	for ( arg = args ; *arg ; arg++ )
		len += ( 1 /* possible space */ + strlen ( *arg ) );

	/* Allocate string */
	string = zalloc ( len );
	if ( ! string )
		return NULL;

	/* Populate string */
	ptr = string;
	for ( arg = args ; *arg ; arg++ ) {
		ptr += sprintf ( ptr, "%s%s",
				 ( ( ptr == string ) ? "" : " " ), *arg );
	}
	assert ( ptr < ( string + len ) );

	return string;
}

/** "echo" options */
struct echo_options {
	/** Do not print trailing newline */
	int no_newline;
};

/** "echo" option list */
static struct option_descriptor echo_opts[] = {
	OPTION_DESC ( "n", 'n', no_argument,
		      struct echo_options, no_newline, parse_flag ),
};

/** "echo" command descriptor */
static struct command_descriptor echo_cmd =
	COMMAND_DESC ( struct echo_options, echo_opts, 0, MAX_ARGUMENTS,
		       "[-n] [...]" );

/**
 * "echo" command
 *
 * @v argc		Argument count
 * @v argv		Argument list
 * @ret rc		Return status code
 */
static int echo_exec ( int argc, char **argv ) {
	struct echo_options opts;
	char *text;
	int rc;

	/* Parse options */
	if ( ( rc = parse_options ( argc, argv, &echo_cmd, &opts ) ) != 0 )
		return rc;

	/* Parse text */
	text = concat_args ( &argv[optind] );
	if ( ! text )
		return -ENOMEM;

	/* Print text */
	printf ( "%s%s", text, ( opts.no_newline ? "" : "\n" ) );

	free ( text );
	return 0;
}

/** "echo" command */
struct command echo_command __command = {
	.name = "echo",
	.exec = echo_exec,
};

/** "exit" options */
struct exit_options {};

/** "exit" option list */
static struct option_descriptor exit_opts[] = {};

/** "exit" command descriptor */
static struct command_descriptor exit_cmd =
	COMMAND_DESC ( struct exit_options, exit_opts, 0, 1, "[<status>]" );

/**
 * "exit" command
 *
 * @v argc		Argument count
 * @v argv		Argument list
 * @ret rc		Return status code
 */
static int exit_exec ( int argc, char **argv ) {
	struct exit_options opts;
	unsigned int exit_code = 0;
	int rc;

	/* Parse options */
	if ( ( rc = parse_options ( argc, argv, &exit_cmd, &opts ) ) != 0 )
		return rc;

	/* Parse exit status, if present */
	if ( optind != argc ) {
		if ( ( rc = parse_integer ( argv[optind], &exit_code ) ) != 0 )
			return rc;
	}

	/* Stop shell processing */
	shell_stop ( SHELL_STOP_COMMAND_SEQUENCE );

	return exit_code;
}

/** "exit" command */
struct command exit_command __command = {
	.name = "exit",
	.exec = exit_exec,
};

/** "isset" options */
struct isset_options {};

/** "isset" option list */
static struct option_descriptor isset_opts[] = {};

/** "isset" command descriptor */
static struct command_descriptor isset_cmd =
	COMMAND_DESC ( struct isset_options, isset_opts, 0, MAX_ARGUMENTS,
		       "[...]" );

/**
 * "isset" command
 *
 * @v argc		Argument count
 * @v argv		Argument list
 * @ret rc		Return status code
 */
static int isset_exec ( int argc, char **argv ) {
	struct isset_options opts;
	int rc;

	/* Parse options */
	if ( ( rc = parse_options ( argc, argv, &isset_cmd, &opts ) ) != 0 )
		return rc;

	/* Return success iff any arguments exist */
	return ( ( optind == argc ) ? -ENOENT : 0 );
}

/** "isset" command */
struct command isset_command __command = {
	.name = "isset",
	.exec = isset_exec,
};
