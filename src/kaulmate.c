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
#include "libtwirc.h"

#define NAME   "kaulmate"
#define AUTHOR "domsson"
#define URL    "https://github.com/domsson/kaulmate"

#define VERSION_MAJOR 0
#define VERSION_MINOR 2
#ifndef BUILD
  #define BUILD 0.0
#endif

#define TOKEN_SIZE 1024
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
	// login
	char *host;
	char *port;
	char *chan;
	char *nick;
	char *pass;
	
	// stuff
	char *owner;

	// social
	char *youtube;

	// other stuff
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
	return (get_time() - cfg->last_msg) >= MSG_INTERVAL;
}

int send_msg(struct twirc_state *s, twirc_event_t *evt, const char *msg)
{
	struct kaul_config *cfg = twirc_get_context(s);
	double now = get_time();
	double delta = now - cfg->last_msg;

	if (delta < MSG_INTERVAL)
	{
		fprintf(stderr, "Did not send message, last message was just %.2f s ago.\n", delta);
		return 1;
	}

	if (strcmp(evt->command, "WHISPER") == 0)
	{
		twirc_cmd_whisper(s, evt->origin, msg);
	}
	else
	{
		twirc_cmd_privmsg(s, evt->channel, msg);
	}
	cfg->last_msg = now;
	return 0;
}

void cmd_bot(struct twirc_state *s, twirc_event_t *evt)
{
	char msg[1024];
	snprintf(msg, 1024, "I'm %s, version %o.%o build %f by %s, see %s",
			(twirc_get_login(s))->nick,
			VERSION_MAJOR,
			VERSION_MINOR,
			BUILD,
			AUTHOR,
			URL);
	send_msg(s, evt, msg);
}

int random_line(const char *file, char *buf, size_t len)
{
	if (file == NULL)
	{
		return -1;
	}

	FILE *fp = fopen(file, "r");
	if (fp == NULL)
	{
		fprintf(stderr, "Could not open file: %s\n", file);
		return -1;
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

	for (int i = 0; fgets(buf, len, fp) != NULL; ++i)
	{
		if (i == line)
		{
			break;
		}
	}
	fclose(fp);

	return 0;
}

void cmd_random_color(struct twirc_state *s, twirc_event_t *evt)
{
	char col[64];
	random_line("named-colors", col, 64);
	twirc_cmd_color(s, col);
}

void cmd_random(struct twirc_state *s, twirc_event_t *evt)
{
   	char str[2048];
	random_line("phrases", str, 2048);
	send_msg(s, evt, str);
}

void cmd_time(struct twirc_state *s, twirc_event_t *evt)
{
	// time_t is not guaranteed to be an int so it isn't
	// exactly safe to add to it, but it seems safe enough.
	
	struct kaul_config *cfg = twirc_get_context(s);
	const time_t t = time(NULL) + (cfg->timezone * 60 * 60);
	struct tm *gmt = gmtime(&t);

	char timestr[32];
	snprintf(timestr, 32, "Current time: %02d:%02d (GMT+%d)", gmt->tm_hour, gmt->tm_min, cfg->timezone);
	send_msg(s, evt, timestr);
}

void cmd_youtube(struct twirc_state *s, twirc_event_t *evt)
{
	struct kaul_config *cfg = twirc_get_context(s);
	send_msg(s, evt, cfg->youtube);
}

void cmd_vanish(twirc_state_t *s, twirc_event_t *evt)
{
	twirc_cmd_timeout(s, evt->channel, evt->origin, 1, NULL);
}

void cmd_slashcock(twirc_state_t *s, twirc_event_t *evt)
{
	send_msg(s, evt, "Did you know that the first ever subscriber was cockeys? And did you know that the first ever subscriber was fuwawame, but the sub was actually a giftsub by SlashLife? Both on 2019-03-08. Well, you know now!");
}

void cmd_pixelogic(twirc_state_t *s, twirc_event_t *evt)
{
	send_msg(s, evt, "Did you know that the first big (22 users!) raid was by PixelogicDev on 2019-03-08!");
}

void cmd_sit(twirc_state_t *s, twirc_event_t *evt)
{
	send_msg(s, evt, "Get up! Stand up! Sitting for too long will kill you :-(");
}

void cmd_victor(twirc_state_t *s, twirc_event_t *evt)
{
	send_msg(s, evt, "Did you know that on 2019-03-31, domsson got his 100th follower? Yes, that's right! It was victornizeyimana, yay!");
}

void handle_command(struct twirc_state *s, twirc_event_t *evt, const char *cmd)
{
	if (strcmp(cmd, "!bot") == 0)
	{
		cmd_bot(s, evt);
		return;
	}
	if (strcmp(cmd, "!time") == 0)
	{
		cmd_time(s, evt);
		return;
	}
	if (strcmp(cmd, "!youtube") == 0)
	{
		cmd_youtube(s, evt);
		return;
	}
	if (strcmp(cmd, "!yt") == 0)
	{
		cmd_youtube(s, evt);
		return;
	}
	if (strcmp(cmd, "!slashcock") == 0 || strcmp(cmd, "!cocklife") == 0)
	{
		cmd_slashcock(s, evt);
		return;
	}
	if (strcmp(cmd, "!victor") == 0)
	{
		cmd_victor(s, evt);
		return;
	}
	if (strcmp(cmd, "!pixelogic") == 0)
	{
		cmd_pixelogic(s, evt);
		return;
	}
	if (strcmp(cmd, "!color") == 0)
	{
		struct kaul_config *cfg = twirc_get_context(s);
		if (cfg->owner == NULL)
		{
			return;
		}
		if (strcmp(evt->origin, cfg->owner) == 0)
		{
			cmd_random_color(s, evt);
		}
		return;
	}
	if (strcmp(cmd, "!sit") == 0)
	{
		cmd_sit(s, evt);
		return;
	}
	if (strcmp(cmd, "!oops") == 0)
	{
		cmd_vanish(s, evt);
		return;
	}
	if (strcmp(cmd, "!drugs") == 0)
	{
		send_msg(s, evt, "Say no to drugs!");
		return;
	}
	if (strcmp(cmd, "!pegi") == 0)
	{
		send_msg(s, evt, "Itz totally family friendly, I swear! l0rn");
		return;
	}
	if (strcmp(cmd, "!music") == 0)
	{
		send_msg(s, evt, "Check out http://www.planettobor.com/music :-)");
		return;
	}

	cmd_random(s, evt);
}

void event_welcome(struct twirc_state *s, struct twirc_event *evt)
{
	fprintf(stderr, "*** connected\n");
	struct kaul_config *cfg = twirc_get_context(s);
	twirc_cmd_join(s, cfg->chan);
}

/*
 * Someone joined the channel - it could be us!
 */
void event_join(struct twirc_state *s, struct twirc_event *evt)
{
	struct kaul_config *cfg = twirc_get_context(s);

	if (strcmp(evt->origin, cfg->nick) == 0)
	{
		fprintf(stderr, "*** we joined %s\n", evt->channel);

		if (strcmp(evt->channel, cfg->chan) == 0)
		{
			//twirc_cmd_privmsg(s, cfg->chan, "jobruce is the best!");
		}
	}
}

void event_privmsg(struct twirc_state *s, struct twirc_event *evt)
{
	fprintf(stdout, "%s: %s\n", evt->origin, evt->message);

	if (evt->message[0] == '!')
	{
		handle_command(s, evt, evt->message);
	}
}

void event_whisper(struct twirc_state *s, struct twirc_event *evt)
{
	fprintf(stdout, "(WHISPER) %s: %s\n", evt->origin, evt->message);

	if (evt->message[0] == '!')
	{
		handle_command(s, evt, evt->message);
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
	
	if (str_equals(name, "owner") || str_equals(name, "owner"))
	{
		cfg->owner = str_quoted(value) ? str_unquote(value) : strdup(value);
		return 1;
	}

	if (str_equals(name, "timezone"))
	{
		cfg->timezone = atoi(value);
		return 1;
	}

	if (str_equals(name, "youtube"))
	{
		cfg->youtube = str_quoted(value) ? str_unquote(value) : strdup(value);
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

	struct twirc_state *s = twirc_init();

	if (!s)
	{
		fprintf(stderr, "Could not initiate IRC session, aborting.\n");
		return EXIT_FAILURE;
	}

	struct twirc_callbacks *cbs = twirc_get_callbacks(s);
	
	cbs->welcome = event_welcome;
	cbs->privmsg = event_privmsg;
	cbs->whisper = event_whisper;
	cbs->join    = event_join;

	if (twirc_connect(s, cfg.host, cfg.port, cfg.nick, cfg.pass) != 0)
	{
		fprintf(stderr, "Could not connect to IRC\n");
		return EXIT_FAILURE;
	}

	twirc_set_context(s, &cfg);

	twirc_loop(s);
	free_cfg(&cfg);
	twirc_kill(s);

	fprintf(stderr, "Good bye!\n");
	return EXIT_SUCCESS;
}
