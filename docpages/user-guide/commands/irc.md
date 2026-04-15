\page irc irc program (satellite)

```basic
irc [nickname] [hostname[:port]] [GECOS]
```

The Satellite IRC client. This is a simple IRC client specifically made for Retro Rocket OS.

\image html irc.png

If you do not specify a nickname, a random nickname will be allocated. This random nickname will be `Rocketeer` followed by 3 digits.

If you do not specify a hostname to connect to, the program will connect to `irc.chatspike.net`.

If the GECOS field is left empty, then the default `Satellite IRC Client` will be used.

If a port is not specified, the default IRC port `6667` will be used.

If DNS resolution fails, Satellite will attempt to connect using the hostname as an IP address.

## Basic usage

After connecting, Satellite shows incoming server messages, channel messages, notices, joins, parts, quits, kicks, topics, user lists, and other IRC numerics.

The current target channel is shown in square brackets on the input line at the bottom of the screen.

Type a line and press Enter to send it to the current channel.

If the input begins with `/`, it is treated as a command instead of a message.

## Keyboard controls

* **Enter** sends the current line or runs a slash command
* **Esc** quits the client
* **Left / Right** move the input cursor
* **Home / End** move to the start or end of the input line
* **Backspace** deletes the character to the left of the cursor
* **Delete** deletes the character to the right of the cursor
* **Page Up / Page Down** scroll through scrollback
* **Tab** inserts `#`

## Channel handling

Satellite keeps track of the channels you have joined.

When you join a channel yourself, it becomes the current channel automatically.

When you leave or are kicked from the current channel, Satellite selects another joined channel if one is available. If not, it falls back to the network name or `-`.

## Commands

### /join

Join a channel.

```basic
/join #channel
```

### /part

Leave a channel.

```basic
/part #channel
```

### /channel

Switch the current input target to a channel you have already joined.

```basic
/channel #channel
```

### /channels

Show the list of joined channels.

```basic
/channels
```

The currently selected channel is marked with `*`.

### /msg

Send a private message or message a channel without switching the current channel.

```basic
/msg target message text
```

### /privmsg

Alias of `/msg`.

```basic
/privmsg target message text
```

### /notice

Send an IRC NOTICE.

```basic
/notice target message text
```

### /nick

Change nickname.

```basic
/nick newnick
```

If the nickname is already in use during connection, Satellite will automatically try the same nickname with an underscore appended.

### /raw

Send a raw IRC line exactly as typed after the command.

```basic
/raw WHOIS someuser
/raw MODE #channel +b
```

### /rawlog

Turn raw server line logging on or off.

```basic
/rawlog on
/rawlog off
```

When enabled, incoming raw IRC lines are shown in the scrollback prefixed with `RAWLOG:`.

### /quit

Disconnect from the server.

```basic
/quit
/quit Gone for tea
```

If no quit message is given, Satellite uses `Satellite IRC Client`.

## What Satellite shows

Satellite shows the following directly in the chat window:

* channel messages
* notices
* joins
* parts
* quits
* kicks
* topic text
* topic setter and time
* NAMES replies
* MOTD
* visible host changes
* common IRC errors such as no such nick, no such channel, cannot send to channel, nickname in use, and not enough parameters
* many standard IRC numerics such as WHOIS, LIST, WHO, INFO, LINKS, ban lists, LUSERS, STATS, ADMIN, TIME and related replies

Unknown numerics are still shown, so important server replies are not silently lost.

## Notes

Satellite uses a single shared scrollback buffer for all output.

`/channel` changes the current input target. It does not create separate per-channel windows.

The client currently uses plaintext IRC connection and does not support TLS.

