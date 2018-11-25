#include <stdio.h>	// printf(), perror(), snprintf(), fopen()
#include <stdlib.h>	// malloc(), free(), getenv(), srandom()
#include <time.h>	// time(), clock_gettime()
#include <string.h>	// strcmp(), strcpy(), strdup(), strcat()
#include <limits.h>	// PATH_MAX
#include <errno.h>	// errno
#include <sys/stat.h>	// stat(), mkdir()
#include <sys/types.h>	// mkdir()
#include "strutils.h"
#include "dirutils.h"
#include "ini.h"
#include "libircclient.h"
#include "libirc_rfcnumeric.h"

#define NAME   "kaulmate"
#define AUTHOR "domsson"
#define URL    "https://github.com/domsson/kaulmate"

#define VERSION_MAJOR 0
#define VERSION_MINOR 2
#ifndef BUILD
  #define BUILD 0.0
#endif

#define TOKEN_SIZE 1024
#define COMMAND_SIZE 32
#define MSG_INTERVAL 1.5 

#define CONFIG_GENERAL "config.ini"
#define CONFIG_ACCOUNT "login.ini"

double last_msg;

// Currently not in use, still thinking how I want
// to go about this exactly...
struct irc_context
{
	const struct kaul_config *cfg;
	const float msg_cap;
};

struct kaul_config
{
	char	*host;
	unsigned port;
	char	*chan;
	char	*nick;
	char	*pass;

	char cmdchr;
	int timezone;
};

void free_cfg(struct kaul_config *cfg)
{
	free(cfg->host);
	cfg->host = NULL;
	free(cfg->chan);
	cfg->chan = NULL;
	free(cfg->nick);
	cfg->nick = NULL;
	free(cfg->pass);
	cfg->pass = NULL;
}

double get_time()
{
	clockid_t cid = CLOCK_MONOTONIC;
	// TODO the next line is cool, as CLOCK_MONOTONIC is not
	// present on all systems, where CLOCK_REALTIME is, however
	// I don't want to call sysconf() with every single iteration
	// of the main loop, so let's do this ONCE and remember...
	//clockid_t cid = (sysconf(_SC_MONOTONIC_CLOCK) > 0) ? CLOCK_MONOTONIC : CLOCK_REALTIME;
	struct timespec ts;
	clock_gettime(cid, &ts);
	return (double) ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

/*
 * Returns 1 if enough time has passed since the last message
 * in order not to 'spam', otherwise 0.
 */
int can_send()
{
	return (get_time() - last_msg) >= MSG_INTERVAL;
}

int send_msg(irc_session_t *s, const char *msg)
{
	double now = get_time();
	double delta = now - last_msg;

	if (delta < MSG_INTERVAL)
	{
		fprintf(stderr, "Did not send message, last message was just %.2f s ago.\n", delta);
		return 1;
	}

	struct kaul_config *cfg = irc_get_ctx(s);
	if (irc_cmd_msg(s, cfg->chan, msg))
	{
		fprintf(stderr, "Could not send message. Is the connection still up?\n");
		return 1;
	}

	last_msg = now;
	return 0;
}

void cmd_bot(irc_session_t *s)
{
	char msg[1024];
	snprintf(msg, 1024, "I'm kaulmate, version %o.%o build %f by %s, see %s",
			VERSION_MAJOR,
			VERSION_MINOR,
			BUILD,
			AUTHOR,
			URL);
	send_msg(s, msg);
}

void cmd_random(irc_session_t *s)
{
	FILE *fp = fopen("random", "r");
	if (fp == NULL)
	{
		fprintf(stderr, "Could not open 'random' file\n");
		return;
	}
	unsigned lines = 0;
	int ch = 0;

	while((ch = fgetc(fp)) != EOF)
	{
		if (ch == '\n')
		{
			lines++;
		}
	}

	fseek(fp, 0, SEEK_SET);
	
	int line = random() % lines;
   	char str[2048];

	int i = 0;
	for (int i = 0; fgets(str, 2048, fp) != NULL; ++i) {
		if (i == line)
		{
			break;
		}
	}
	fclose(fp);

	send_msg(s, str);
}

void cmd_time(irc_session_t *s)
{
	// time_t is not guaranteed to be an int so it isn't
	// exactly safe to add to it, but it seems safe enough.
	struct kaul_config *cfg = irc_get_ctx(s);
	const time_t t = time(NULL) + (cfg->timezone * 60 * 60);
	struct tm *gmt = gmtime(&t);

	char timestr[32];
	snprintf(timestr, 32, "Current time: %02d:%02d (GMT+%d)", gmt->tm_hour, gmt->tm_min, cfg->timezone);
	send_msg(s, timestr);
	return;
}

void cmd_youtube(irc_session_t *s)
{
	// TODO obviously this will have to come out of a config file
	const char yt[] = "https://www.youtube.com/channel/UCNYkFdQfHWKnTr6ko9XcJig";
	send_msg(s, yt);
}

void handle_command(irc_session_t *s, const char *cmd)
{
	if (strcmp(cmd, "!bot") == 0)
	{
		cmd_bot(s);
		return;
	}
	if (strcmp(cmd, "!time") == 0)
	{
		cmd_time(s);
		return;
	}
	if (strcmp(cmd, "!youtube") == 0)
	{
		cmd_youtube(s);
		return;
	}
	if (strcmp(cmd, "!yt") == 0)
	{
		cmd_youtube(s);
		return;
	}
	cmd_random(s);
}

/*
 * This is our dummy event handler that we'll assign to events we're not interested in.
 */
void event_null(irc_session_t *s, const char *e, const char *o, const char **p, unsigned c)
{
	printf("event_null\n");
	if (e)   printf("\tevent   = %u\n", e);
	if (o)   printf("\torigin  = %s\n", o);
	if (c>0) printf("\tparam 1 = %s\n", p[0]);
	if (c>1) printf("\tparam 2 = %s\n", p[1]);
	if (c>2) printf("\tparam 3 = %s\n", p[2]);
}

/*
 * Fired once we established a connection with the server.
 * This would be a good place to kick off other actions (joining channel etc)
 */
void event_connect(irc_session_t *s, const char *e, const char *o, const char **p, unsigned c)
{
	printf("event_connect\n");
	if (e)  printf("\tevent   = %s\n", e);
	if (o)  printf("\torigin  = %s\n", o);

	struct kaul_config *cfg = irc_get_ctx(s);

	// TODO: figure out if we actually need this CAP at this time
	if (irc_send_raw(s, "CAP REQ :twitch.tv/membership"))
	{
		fprintf(stderr, "Could not request membership capability.\n");
	}

	/*
	 * TODO we don't request tags capability at this point, because
	 * tags are a IRCv3 feature that isn't supported by libircclient.
	 * We'll have to look for a different library, write our own or,
	 * maybe even better, patch libircclient and send a push request.
	 */
	/*
	if (irc_send_raw(s, "CAP REQ :twitch.tv/tags"))
	{
		fprintf(stderr, "Could not request tags capability.\n");
	}
	*/

	// TODO: figure out if we actually need this CAP at this time
	if (irc_send_raw(s, "CAP REQ :twitch.tv/commands"))
	{
		fprintf(stderr, "Could not request commands capability.\n");
	}

	if (irc_cmd_join(s, cfg->chan, 0))
	{
		fprintf(stderr, "Could not join channel %s\n", cfg->chan);
	}
}

/*
 * Someone joined the channel - it could be us!
 */
void event_join(irc_session_t *s, const char *e, const char *o, const char **p, unsigned c)
{
	printf("event_join\n");
	if (e)   printf("\tevent   = %s\n", e);
	if (o)   printf("\torigin  = %s\n", o);
	if (c>0) printf("\tchannel = %s\n", p[0]);
}

/*
 * Someone left the channel - it could be us!
 */
void event_part(irc_session_t *s, const char *e, const char *o, const char **p, unsigned c)
{
	printf("event_part\n");
	if (e)   printf("\tevent   = %s\n", e);
	if (o)   printf("\torigin  = %s\n", o);
	if (c>0) printf("\tchannel = %s\n", p[0]);
}

/*
 * Private messages - however, whispers don't seem to get through here
 */
void event_privmsg(irc_session_t *s, const char *e, const char *o, const char **p, unsigned c)
{
	printf("event_privmsg\n");
	if (e)   printf("\tevent   = %s\n", e);
	if (o)   printf("\torigin  = %s\n", o);
	if (c>0) printf("\treceip. = %s\n", p[0]);
	if (c>1) printf("\tmessage = %s\n", p[1]);
}

/*
 * Handler for receiving notices - not sure this is relevant on Twitch.
 */
void event_notice(irc_session_t *s, const char *e, const char *o, const char **p, unsigned c)
{
	printf("event_notice\n");
	if (e)   printf("\tevent   = %s\n", e);
	if (o)   printf("\torigin  = %s\n", o);
	if (c>0) printf("\treceip. = %s\n", p[0]);
	if (c>1) printf("\tmessage = %s\n", p[1]);
}

/*
 * This is where we intercept user's chat messages
 */
void event_channel(irc_session_t *s, const char *e, const char *o, const char **p, unsigned c)
{
	printf("event_channel\n");
	if (e)   printf("\tevent   = %s\n", e);
	if (o)   printf("\torigin  = %s\n", o);
	if (c>0) printf("\tchannel = %s\n", p[0]);
	if (c>1) printf("\tmessage = %s\n", p[1]);

	if (p[1][0] == '!') // This might be a command!
	{
		handle_command(s, p[1]);
	}
}

/*
 * This is the event that triggers for things like PING
 */
void event_ctcp_rep(irc_session_t *s, const char *e, const char *o, const char **p, unsigned c)
{
	printf("event_ctcp_rep\n");
	if (e)   printf("\tevent   = %s\n", e);
	if (o)   printf("\torigin  = %s\n", o);
	if (c>0) printf("\tchannel = %s\n", p[0]);
	if (c>1) printf("\tmessage = %s\n", p[1]);
}

/*
 * This is the event that triggers for /me messages
 */
void event_ctcp_action(irc_session_t *s, const char *e, const char *o, const char **p, unsigned c)
{
	printf("event_ctcp_action\n");
	if (e)   printf("\tevent   = %s\n", e);
	if (o)   printf("\torigin  = %s\n", o);
	if (c>0) printf("\tchannel = %s\n", p[0]);
	if (c>1) printf("\tmessage = %s\n", p[1]);
}

/*
 * These are rather specific events that we (mostly) do not care about
 */
void event_numeric(irc_session_t *s, unsigned e, const char *o, const char **p, unsigned c)
{
	printf("event_numeric\n");
	if (e)   printf("\tevent   = %u\n", e);
	if (o)   printf("\torigin  = %s\n", o);
	if (c>0) printf("\tparam 1 = %s\n", p[0]);
	if (c>1) printf("\tparam 2 = %s\n", p[1]);
	if (c>2) printf("\tparam 3 = %s\n", p[2]);
}
/*
 * Close session, disconnect, free objects, etc
 */
void cleanup(irc_session_t *s)
{
	if (!s)
	{
		return;
	}

	struct kaul_config *cfg = irc_get_ctx(s);
	free_cfg(cfg);

	if (irc_is_connected(s))
	{	
		irc_cmd_quit(s, NULL);
	}

	if (s)
	{
		irc_destroy_session(s);
	}
}

static int cfg_handler(void *config, const char *section, const char *name, const char *value)
{
	struct kaul_config *cfg = (struct kaul_config*) config;

	if (str_equals(name, "host") || str_equals(name, "server"))
	{
		cfg->host = str_quoted(value) ? str_unquote(value) : strdup(value);
		return 1;
	}
	
	if (str_equals(name, "port"))
	{
		cfg->port = atoi(value);
		return 1;
	}

	if (str_equals(name, "nick") || str_equals(name, "nickname"))
	{
		cfg->nick = str_quoted(value) ? str_unquote(value) : strdup(value);
		return 1;
	}

	if (str_equals(name, "pass") || str_equals(name, "password"))
	{
		cfg->pass = str_quoted(value) ? str_unquote(value) : strdup(value);
		return 1;
	}

	if (str_equals(name, "chan") || str_equals(name, "channel"))
	{
		cfg->chan = str_quoted(value) ? str_unquote(value) : strdup(value);
		return 1;
	}
	
	if (str_equals(name, "timezone"))
	{
		cfg->timezone = atoi(value);
		return 1;
	}

	// Unknown section/name, error
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <profile>\n", argv[0]);
		return EXIT_FAILURE;
	}

	fprintf(stderr, "Starting up kaulmate.\n");

	srandom(time(NULL));

	/*
	 * Config directory
	 */

	char cfg_dir[PATH_MAX] = "\0";
	config_dir(cfg_dir, PATH_MAX, NAME);
	fprintf(stderr, "Using config directory: %s\n", cfg_dir);

	if (!dir_ensure(cfg_dir, NULL))
	{
		fprintf(stderr, 
			"Could not access or create config directory: %s (%s)\n",
			cfg_dir, strerror(errno));
		return EXIT_FAILURE;
	}

	dir_concat(cfg_dir, PATH_MAX, argv[1]);

	if (!dir_exists(cfg_dir))
	{
		fprintf(stderr,
			"Could not access profile directory: %s (%s)\n",
			cfg_dir, strerror(errno));
		return EXIT_FAILURE;
	}
	
	char general_cfg_path[PATH_MAX];
	char account_cfg_path[PATH_MAX];

	strcpy(general_cfg_path, cfg_dir);
	strcpy(account_cfg_path, cfg_dir);

	dir_concat(general_cfg_path, PATH_MAX, CONFIG_GENERAL);
	dir_concat(account_cfg_path, PATH_MAX, CONFIG_ACCOUNT);

	struct kaul_config cfg = {};

	if (ini_parse(general_cfg_path, cfg_handler, &cfg) < 0)
	{
		fprintf(stderr,
			"Could not load general config file: %s\n",
			general_cfg_path);
		return EXIT_FAILURE;
	}

	if (ini_parse(account_cfg_path, cfg_handler, &cfg) < 0)
	{
		fprintf(stderr,
			"Could not load account config file: %s\n",
			account_cfg_path);
		return EXIT_FAILURE;
	}

	// TODO incorporate that into the cfg or the ctx or whatever
	last_msg = 0.0;

	if (cfg.host == NULL)
	{
		fprintf(stderr,
			"IRC server ('host') not specified in %s, aborting.\n",
			CONFIG_GENERAL);
		return EXIT_FAILURE;
	}

	if (cfg.port == 0)
	{
		fprintf(stderr, 
			"IRC port ('port') not specified in %s, using default.\n", 
			CONFIG_GENERAL);
		cfg.port = (strchr(cfg.host, ':') == 0) ? 6667 : 0;
	}

	if (cfg.nick == NULL)
	{
		fprintf(stderr,
			"Nickname ('nick') not specified in %s, aborting.\n",
			CONFIG_ACCOUNT);
		return EXIT_FAILURE;
	}

	if (cfg.pass == NULL)
	{
		fprintf(stderr,
			"Password/token ('pass') not specified in %s, aborting.\n",
			CONFIG_ACCOUNT);
		return EXIT_FAILURE;
	}

	// The IRC callbacks structure
	irc_callbacks_t callbacks;

	// Init it (is this really needed?)
	memset(&callbacks, 0, sizeof(callbacks));

	// Set up the event callbacks
	callbacks.event_connect =  event_connect;
	callbacks.event_join =     event_join;
	callbacks.event_nick =     event_null;
	callbacks.event_quit =     event_null;
	callbacks.event_part =     event_part;
	callbacks.event_mode =     event_null;
	callbacks.event_topic =    event_null;
	callbacks.event_kick =     event_null;
	callbacks.event_channel =  event_channel;
	callbacks.event_privmsg =  event_privmsg;
	callbacks.event_notice =   event_notice;
	callbacks.event_invite =   event_null;
	callbacks.event_umode =    event_null;
	callbacks.event_unknown =  event_null;
	callbacks.event_numeric =  event_numeric;

	callbacks.event_ctcp_rep =     event_ctcp_rep;
	callbacks.event_ctcp_action =  event_ctcp_action;
	//callbacks.event_dcc_chat_req = event_null_dcc;
	//callbacks.event_dcc_send_req = event_null_dcc;

	// Now create the session
	irc_session_t  *session;
	session = irc_create_session(&callbacks);

	if (!session)
	{
		fprintf(stderr, "Could not initiate IRC session, aborting.\n");
		return EXIT_FAILURE;
	}

	// Have libirc turn complex nick strings into nicks
	irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);

	// Save the context info into the session for easy retrieval later
	irc_set_ctx(session, &cfg);

	// Initiate the IRC server connection
	if (irc_connect(session, cfg.host, cfg.port, cfg.pass, cfg.nick, cfg.nick, cfg.nick))
	{
		fprintf(stderr, "Could not connect: %s\n", irc_strerror(irc_errno(session)));
		cleanup(session);
		return EXIT_FAILURE;
	}
	
	// Run endless loop that will fire events as they occur
	if (irc_run(session))
	{
		fprintf(stderr, "Could not connect or I/O error: %s\n", irc_strerror(irc_errno(session)));
		cleanup(session);
		return EXIT_FAILURE;
	}

	cleanup(session);
	fprintf(stderr, "Good bye!\n");
	return EXIT_SUCCESS;
}
