# kaulmate

A **very** simple IRC bot written in C. Currently developed on an for Linux. It is supposed to be used with Twitch, though I assume it would work on any IRC server, really. This is mainly a fun project to practice my C skills. I don't plan to add many features.

## Features

Currently, all the bot has to offer are some hard-coded commands (for example `!time`). This means that if you wanted to change the output of those commands (and you most certainly do), then you'd have to dig into the source code.

## Dependencies

- `libircclient` (available in the Debian packages as `libircclient1`, `libircclient-dev`)

## Build

- `chmod +x ./build`
- `./build`

## Set up

You can create multiple profile for the bot, each with its own settings and login credentials. The name of a profile could be your Twitch name, but for the remainder of this guide, we will assume that we're going to create a profile called `default`. You can create as many profiles as you want.

- In your config directory (usually `~/.config`), create a `kaulmate` directory
- In the newly created `kaulmate` directory, create a `default` directory
- In the `default` directory, create two files, `config.ini` and `login.ini`

### `config.ini`

This file holds the general configuration. The two required fields are `host` and `chan`. Example:

    host = irc.chat.twitch.tv
    chan = #domsson

### `login.ini`

This file holds the required fields `nick` (Twich user name) and `pass` ([OAuth2 token](https://twitchapps.com/tmi/)) for your bot. Example:

    nick = kaulmate
    pass = oauth:123456789abcdefghi

## Run

Start up kaulmate, specifying the profile you want to use:

- `bin/kaulmate default`
 
