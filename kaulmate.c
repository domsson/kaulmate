#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include "libircclient.h"
#include "libirc_rfcnumeric.h"

#define NAME "kaulmate"
#define VERSION_MAJOR 0
#define VERSION_MINOR 1

#ifndef VERSION
  #define VERSION 0.0
#endif

#define TOKEN_SIZE 1024
#define COMMAND_SIZE 32
#define MSG_INTERVAL 1.5 
#define TIMEZONE 9

double last_msg;

struct irc_context
{
	const char *host;
	unsigned    port;
	const char *chan;
	const char *nick;
	const char *pass;
};

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

	struct irc_context *context = irc_get_ctx(s);
	if (irc_cmd_msg(s, context->chan, msg))
	{
		fprintf(stderr, "Could not send message. Is the connection still up?\n");
		return 1;
	}

	last_msg = now;
	return 0;
}

void cmd_bot(irc_session_t *s)
{
	printf("v. %o.%o build %f\n", VERSION_MAJOR, VERSION_MINOR, VERSION);

	send_msg(s, "This is kaulmate by domsson - https://github.com/domsson/kaulmate");
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
	//fprintf(stderr, "Number of lines: %u\n", lines);
	
	int line = rand() % lines;
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
	const time_t t = time(NULL) + (TIMEZONE * 60 * 60);
	struct tm *gmt = gmtime(&t);

	char timestr[32];
	snprintf(timestr, 32, "Current time: %02d:%02d (GMT+%d)", gmt->tm_hour, gmt->tm_min, TIMEZONE);
	send_msg(s, timestr);
	return;
}

void cmd_youtube(irc_session_t *s)
{
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
 * Fired once we established connection with the server.
 * This would be a good place to kick of other actions (joining channel etc)
 */
void event_connect(irc_session_t *s, const char *e, const char *o, const char **p, unsigned c)
{
	printf("event_connect\n");
	if (e)  printf("\tevent   = %s\n", e);
	if (o)  printf("\torigin  = %s\n", o);

	struct irc_context *ctx = irc_get_ctx(s);

	if (irc_cmd_join(s, ctx->chan, 0))
	{
		printf("Could not join channel");
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
void event_numeric(irc_session_t *s, unsigned *e, const char *o, const char **p, unsigned c)
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
	if (irc_is_connected(s))
	{	
		irc_cmd_quit(s, NULL);
	}
	if (s)
	{
		irc_destroy_session(s);
	}
}

/*
 * Returns a pointer to a string that holds the config dir we want to use.
 * `sub_dir` is the desired subdirectory within the user's condif dir.
 * This does not check if the dir actually exists. You need to check still.
 * The string is allocated with malloc() and needs to be freed by the caller.
 * If memory allocation for the string fails, NULL will be returned.
 */
char *config_dir(const char *subdir)
{
	// Get the user's home dir - this should be set
	char *home = getenv("HOME");
	// Get the user's config dir - this might not be set
	char *cfg_home = getenv("XDF_CONFIG_HOME");
	// String to hold the complete path to the config dir
	char *cfg_dir = NULL;
	// Length of `cfg_dir`
	size_t cfg_dir_len;

	// In case XDF_CONFIG_HOME was not set, we assume a .config directory
	if (cfg_home == NULL)
	{
		cfg_dir_len = strlen(home) + strlen(".config") + strlen(subdir) + 3;
		cfg_dir = malloc(cfg_dir_len);
		snprintf(cfg_dir, cfg_dir_len, "%s/%s/%s", home, ".config", subdir);
	}
	// If XDF_CONFIG_HOME is set, we'll use that
	else
	{
		cfg_dir_len = strlen(cfg_home) + strlen(subdir) + 2;
		cfg_dir = malloc(cfg_dir_len);
		snprintf(cfg_dir, cfg_dir_len, "%s/%s", cfg_home, subdir);
	}

	return cfg_dir;
}


/*
 * TODO: the comment
 */
char *profile_dir(const char *profile, const char *config_dir)
{
	size_t dir_len = strlen(config_dir) + strlen(profile) + 2;
	char *profile_dir = malloc(dir_len);
	snprintf(profile_dir, dir_len, "%s/%s", config_dir, profile);
	return profile_dir;
}

/*
 * Returns 1 if the given dir exists and is a dir.
 * Returns 0 if the given dir is not a dir or some error occured.
 */
int dir_exists(const char *dir)
{
	struct stat s;
	if (stat(dir, &s) == -1)
	{
		return 0;
	}		
	if (S_ISDIR(s.st_mode))
	{
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		fprintf(stderr, "Usage: %s <server> <channel> <nick>\n", argv[0]);
		return EXIT_FAILURE;
	}

	fprintf(stderr, "Starting up kaulmate.\n");

	char *cfg_dir = config_dir(NAME);
	printf("config dir:  %s\n", cfg_dir);
	if (dir_exists(cfg_dir))
	{
		printf("config dir exists!\n");
	}
	char *prf_dir = profile_dir("domsson", cfg_dir);
	printf("profile dir: %s\n", prf_dir);
	free(cfg_dir);
	free(prf_dir);

	FILE *fp;
	char token[TOKEN_SIZE];

	/* opening file for reading */
	fp = fopen("token" , "r");
	if (fp == NULL) {
		perror("Error opening token file");
		return EXIT_FAILURE;
	}
	if (fgets(token, TOKEN_SIZE, fp) != NULL) {

	}
	fclose(fp);

	//
	last_msg = 0.0;

	struct irc_context context;

	context.host = argv[1];
	context.port = (strchr(argv[1], ':') == 0) ? 6667 : 0;
	context.chan = argv[2];
	context.nick = argv[3];
	context.pass = token;

	// The IRC callbacks structure
	irc_callbacks_t callbacks;
	irc_session_t *session;

	// Init it (is this needed?)
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

	callbacks.event_ctcp_rep =     event_null;
	callbacks.event_ctcp_action =  event_ctcp_action;
	callbacks.event_dcc_chat_req = event_null;
	callbacks.event_dcc_send_req = event_null;

	// Now create the session
	session = irc_create_session(&callbacks);

	if (!session)
	{
		fprintf(stderr, "Could not initiate IRC session, aborting.\n");
		return EXIT_FAILURE;
	}

	// Have libirc turn complex nick strings into nicks
	irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);

	// Save the context info into the session for easy retrieval later
	irc_set_ctx(session, &context);

	// Initiate the IRC server connection
	if (irc_connect(session, context.host, context.port, context.pass, context.nick, context.nick, context.nick))
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
