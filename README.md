# kaulmate

A **very** simple IRC bot written in C. It is supposed to be used with Twitch, though I assume it would work on any IRC server, really. This is mainly a fun project to practice my C skills. If you're looking for something to use, you should probably check out PhantomBot.

## Features

Currently, all the bot has to offer are some hard-coded commands (for example `!time`). I say hardcoded, because they all have some information (timezone, for example) in the source code that you probably would want to change.

## Dependencies

- `libircclient` (available in the Debian packages as `libircclient1`)

## Build

- `chmod +x ./build`
- `./build`

## Run

- Create a file called `token`
- Paste your bot OAuth token into the file
- `./kaulmate irc.chat.twitch.tv "#channel" botname`
 
