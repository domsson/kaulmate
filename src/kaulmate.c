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
#include "libtwirc/src/libtwirc.h"

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

//double last_msg;

// Currently not in use, still thinking how I want
// to go about this exactly...
/*
struct irc_context
{
	const struct kaul_config *cfg;
	const float msg_cap;
};
*/

struct kaul_config
{
	char *host;
	char *port;
	char *chan;
	char *nick;
	char *pass;

	char cmdchr;
	int timezone;
	float msg_cap;
	double last_msg;
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
int can_send(struct twirc_state *s)
{
	struct kaul_config *cfg = twirc_get_context(s);
	//return (get_time() - last_msg) >= MSG_INTERVAL;
	return (get_time() - cfg->last_msg) >= MSG_INTERVAL;
}

int send_msg(struct twirc_state *s, const char *msg)
{
	struct kaul_config *cfg = twirc_get_context(s);
	double now = get_time();
	//double delta = now - last_msg;
	double delta = now - cfg->last_msg;

	if (delta < MSG_INTERVAL)
	{
		fprintf(stderr, "Did not send message, last message was just %.2f s ago.\n", delta);
		return 1;
	}

	twirc_cmd_privmsg(s, "#domsson", msg);
	//last_msg = now;
	cfg->last_msg = now;
	return 0;
}

void cmd_bot(struct twirc_state *s)
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

void cmd_random(struct twirc_state *s)
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

	for (int i = 0; fgets(str, 2048, fp) != NULL; ++i)
	{
		if (i == line)
		{
			break;
		}
	}
	fclose(fp);

	send_msg(s, str);
}

void cmd_time(struct twirc_state *s)
{
	// time_t is not guaranteed to be an int so it isn't
	// exactly safe to add to it, but it seems safe enough.
	
	struct kaul_config *cfg = twirc_get_context(s);
	const time_t t = time(NULL) + (cfg->timezone * 60 * 60);
	struct tm *gmt = gmtime(&t);

	char timestr[32];
	snprintf(timestr, 32, "Current time: %02d:%02d (GMT+%d)", gmt->tm_hour, gmt->tm_min, cfg->timezone);
	send_msg(s, timestr);
}

void cmd_youtube(struct twirc_state *s)
{
	// TODO obviously this will have to come out of a config file
	const char yt[] = "https://www.youtube.com/channel/UCNYkFdQfHWKnTr6ko9XcJig";
	send_msg(s, yt);
}

void handle_command(struct twirc_state *s, const char *cmd)
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

void event_welcome(struct twirc_state *s, struct twirc_event *evt)
{
	twirc_cmd_join(s, "#domsson");
}

/*
 * Someone joined the channel - it could be us!
 */
void event_join(struct twirc_state *s, struct twirc_event *evt)
{
	if (strcmp(evt->origin, "kaulmate") == 0)
	{
		fprintf(stderr, "*** we joined %s\n", evt->channel);

		if (strcmp(evt->channel, "#domsson") == 0)
		{
			twirc_cmd_privmsg(s, "#domsson", "jobruce is the best!");
		}
	}
}

/*
 * Someone left the channel - it could be us!
 */
void event_part(struct twirc_state *s, struct twirc_event *evt)
{
	if (strcmp(evt->origin, "kaulmate") == 0)
	{
		fprintf(stderr, "*** we left %s\n", evt->channel);
	}
}

void event_privmsg(struct twirc_state *s, struct twirc_event *evt)
{
	fprintf(stdout, "%s: %s\n", evt->origin, evt->message);

	if (evt->message[0] == '!')
	{
		handle_command(s, evt->message);
	}
}

/*
 * This is the event that triggers for /me messages
 */
void event_action(struct twirc_state *s, struct twirc_event *evt)
{
	fprintf(stdout, "* %s %s\n", evt->origin, evt->message);
}

/*
 * Close session, disconnect, free objects, etc
 */
void cleanup(struct twirc_state *s)
{
	if (!s)
	{
		return;
	}

	struct kaul_config *cfg = twirc_get_context(s);
	free_cfg(cfg);

	if (s)
	{
		twirc_kill(s);
	}
}

int cfg_handler(void *config, const char *section, const char *name, const char *value)
{
	struct kaul_config *cfg = (struct kaul_config*) config;

	if (str_equals(name, "host") || str_equals(name, "server"))
	{
		cfg->host = str_quoted(value) ? str_unquote(value) : strdup(value);
		return 1;
	}
	
	if (str_equals(name, "port"))
	{
		cfg->port = str_quoted(value) ? str_unquote(value) : strdup(value);
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

	struct kaul_config cfg = { 0 };

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
	//last_msg = 0.0;

	if (cfg.host == NULL)
	{
		fprintf(stderr,
			"IRC server ('host') not specified in %s, aborting.\n",
			CONFIG_GENERAL);
		return EXIT_FAILURE;
	}

	if (cfg.port == NULL)
	{
		fprintf(stderr, 
			"IRC port ('port') not specified in %s, using default.\n", 
			CONFIG_GENERAL);
		cfg.port = (strchr(cfg.host, ':') == 0) ? "6667" : NULL;
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

	struct twirc_state *s= twirc_init();

	if (!s)
	{
		fprintf(stderr, "Could not initiate IRC session, aborting.\n");
		return EXIT_FAILURE;
	}

	struct twirc_callbacks *cbs = twirc_get_callbacks(s);
	
	cbs->welcome = event_welcome;
	cbs->privmsg = event_privmsg;

	if (twirc_connect(s, cfg.host, cfg.port, cfg.nick, cfg.pass) != 0)
	{
		fprintf(stderr, "Could not connect to IRC\n");
		return EXIT_FAILURE;
	}

	twirc_set_context(s, &cfg);

	twirc_loop(s, 1000);
	free_cfg(&cfg);
	twirc_kill(s);

	fprintf(stderr, "Good bye!\n");
	return EXIT_SUCCESS;
}
