# kaulmate

A **very** simple IRC bot written in C. Currently developed on an for Linux. It is supposed to be used with Twitch, though I assume it would work on any IRC server, really. This is mainly a fun project to practice my C skills. I don't plan to add many features.

## Features

Currently, all the bot has to offer are some hard-coded commands (for example `!time`). This means that if you wanted to change the output of those commands (and you most certainly do), then you'd have to dig into the source code.

## Dependencies

- [`libircclient`](https://github.com/shaoner/libircclient) (available in the Debian packages as `libircclient-dev`)
- [`inih`](https://github.com/benhoyt/inih) (available in the Debian packages as `libinih-dev`)

## Build

The build script assumed that you have `gcc` installed. If you want to use a different compiler, please edit the build script accordingly.

- `chmod +x ./build`
- `./build`

## Set up

You can create multiple profiles for the bot, each with its own settings and login credentials. You are free to name the profiles whatever you want, but for the remainder of this guide, we will assume that we're going to create a profile called `default`.

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

## Licence

kaulmate is free software, dedicated to the public domain. Do with it whatever you want, but don't hold me responsible for anything either.
