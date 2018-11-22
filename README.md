# kaulmate

A **very** simple IRC bot written in C. Currently developed on an for Linux. It is supposed to be used with Twitch, though I assume it would work on any IRC server, really. This is mainly a fun project to practice my C skills. I don't plan to add many features.

## Features

Currently, all the bot has to offer are some hard-coded commands (for example `!time`). This means that if you wanted to change the output of those commands (and you most certainly do), then you'd have to dig into the source code.

## Dependencies

- `libircclient` (available in the Debian packages as `libircclient1`)

## Build

- `chmod +x ./build`
- `./build`

## Run

- Create a file called `token`
- Paste [your bot OAuth token](https://twitchapps.com/tmi/) into the file
- `./kaulmate irc.chat.twitch.tv "#channel" botname`
 
