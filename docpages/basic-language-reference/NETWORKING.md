\page networking BASIC Networking

[TOC]

### Introduction to Sockets

Retro Rocket BASIC is not limited to files, graphics, and local input/output.
It also has direct **network and Internet access**, built into the language itself.
This is provided through **sockets**.

A socket is a numbered channel that lets your program send and receive data across the network. With sockets you can:

* **Build clients** that connect to remote services such as web servers, APIs, or other Retro Rocket programs.
* **Build servers** that listen for incoming connections from other machines, whether on your local network or the wider Internet.
* **Exchange data directly** between two programs on the same computer without using files.

Sockets are deliberately designed to be simple in Retro Rocket BASIC. You do not need to know about retries, buffers, or low-level networking details. A few straightforward keywords are enough to open a connection, send and receive data, and close it again.

This section of the manual introduces sockets, shows how to create both clients and servers, and explains the two kinds of socket Retro Rocket provides:

* **TCP sockets** for reliable, ordered conversations.
* **UDP sockets** for quick, lightweight messages.

Once you understand these, you can write networked applications such as web servers, chat clients, multiplayer games, and telemetry systems - all in BASIC, with the same clarity and brevity as printing text to the screen.

---

### Understanding Sockets (Phone Calls vs Parcels)

Networking in Retro Rocket BASIC uses **sockets**: simple handles (just numbers) that let programs talk over the network. There are two flavours with very different “feel”:

* **TCP** - like a **phone call**: a live conversation with guaranteed order and delivery.
* **UDP** - like posting a **parcel**: each item is sent on its own; most arrive, some may not.

You don’t need to learn object systems, callbacks, or frameworks. The flow is linear: **connect/listen → read/write → flush (if needed) → close**.

---

#### TCP - Like a Phone Call (reliable conversation)

TCP aims to feel like you’ve got a dedicated line once connected.

* \ref CONNECT "CONNECT" - dial the other side. Their phone “rings”.
* \ref SOCKLISTEN "SOCKLISTEN" - keep your phone on, ready to receive calls on a number (port).
* \ref SOCKACCEPT "SOCKACCEPT" - swipe to answer when someone calls. Each accepted call gets a **new** socket handle (a new line).
* \ref SOCKWRITE "SOCKWRITE" - speak; your words are queued exactly as you said them.
* \ref SOCKFLUSH "SOCKFLUSH" - pause for the “**uh-huh**”: make sure what you said is pushed out and acknowledged before carrying on.
* \ref SOCKREAD "SOCKREAD" - listen; you receive complete values (string / integer / real) in the right order.
* \ref SOCKCLOSE "SOCKCLOSE" - hang up; the line is gone.

**What you get with TCP**

* **No missing words**: bytes aren’t lost.
* **Correct order**: bytes arrive exactly as sent.
* **Simple flow**: write → flush → read → close; no retries, no partial-send faff.

**Use TCP for** web pages, chat, control channels, file transfer-anything where every byte matters.

---

#### UDP - Like Sending a Parcel (quick, independent messages)

UDP is fire-and-forget. Each message is its own parcel.

* \ref UDPBIND "UDPBIND" - write your return address on the box and open your doorstep (port). Both sides should **bind** so they have a clear return address and a place to receive.
* \ref UDPWRITE "UDPWRITE" - pack a parcel and drop it at the post office. It **leaves immediately**. There’s no built-in receipt or guarantee.
* \ref UDPREAD "UDPREAD$" - open the door and pick up the **next** parcel on your doorstep.
* \ref UDPLASTIP "UDPLASTIP$" / \ref UDPLASTSOURCEPORT "UDPLASTSOURCEPORT" - read the sender label (who posted this parcel and from which port) **for the parcel you just picked up**.

**What you get with UDP**

* **Speed & simplicity**: no call setup, no flush.
* **Independence**: each message stands alone; send one or many.
* **No guarantees**: a parcel can be delayed, arrive out of order, or never turn up.

**Use UDP for** quick updates, game ticks, “is-alive” beacons, mouse/keyboard telemetry-cases where losing the odd packet doesn’t hurt.

---

### Side-by-side: Phone Call vs Parcel

| Aspect        | **TCP (Phone Call)**                                             | **UDP (Parcel Post)**                                                                             |
| ------------- | ---------------------------------------------------------------- | ------------------------------------------------------------------------------------------------- |
| How you start | `CONNECT` to call; server `SOCKLISTEN` + `SOCKACCEPT` to answer. | Both sides `UDPBIND` a port (their doorstep). No call setup.                                      |
| Identity      | Each accepted TCP socket is a known line between two ends.       | Each parcel carries its own label; use `UDPLASTIP$` / `UDPLASTSOURCEPORT` after `UDPREAD$`. |
| Sending       | `SOCKWRITE` queues words; `SOCKFLUSH` waits for “uh-huh”.        | `UDPSEND`/`UDPWRITE` posts a parcel immediately. No flush.                                        |
| Receiving     | `SOCKREAD` waits for complete data; order guaranteed.            | `UDPREAD$` pops **one** parcel from the doorstep queue; order not guaranteed.                     |
| Delivery      | Guaranteed and ordered.                                          | Best-effort; may be delayed, re-ordered, or lost.                                                 |
| Ending        | `SOCKCLOSE` hangs up the line.                                   | `SOCKCLOSE`/`UDPUNBIND` stops using that doorstep.                                                |

---

### UDP “Doorstep Queue” (how incoming messages are stored)

When your program `UDPBIND`s a port, Retro Rocket keeps an **in-memory queue** of incoming UDP packets for that port-think **parcels on your doorstep**:

* Every new packet is placed at the **back** of the queue for that port.
* Each call to \ref UDPREAD "UDPREAD$ pops **one** packet from the **front** of the queue and returns its contents as a string.
* After a successful \ref UDPREAD "UDPREAD$ the “label” values for that packet are available via:

  * \ref UDPLASTIP "UDPLASTIP$" - sender’s address (the return address).
  * \ref UDPLASTSOURCEPORT "UDPLASTSOURCEPORT" - sender’s source port (their posting office counter).
    These **do not take parameters**: they always refer to the **most recently read** packet.

This model is simple, predictable, and maps perfectly to the “parcel on the doorstep” intuition.

---

### Worked UDP Example - Mouse Telemetry

Below is your exact module, which binds a per-process port and polls a local mouse server. It showcases `UDPBIND`, `UDPWRITE`, `UDPREAD$`, and consuming one packet at a time:

```basic
DEF PROCmouse
    __MOUSE_X = GRAPHICS_WIDTH / 2
    __MOUSE_Y = GRAPHICS_HEIGHT / 2
    __MOUSE_LMB = FALSE
    __MOUSE_RMB = FALSE
    __MOUSE_MMB = FALSE
    UDPBIND "127.0.0.1", 14502 + PID
ENDPROC

DEF PROCmouse_done
    UDPUNBIND "127.0.0.1", 14502 + PID
ENDPROC

DEF PROCfetch_mouse
    UDPWRITE "127.0.0.1", 14502 + PID, 14501, "GET"
    PACKET$ = UDPREAD$(14502 + PID)
    IF PACKET$ <> "" THEN
        X$ = TOKENIZE$(PACKET$, " ")
        Y$ = TOKENIZE$(PACKET$, " ")
        M$ = TOKENIZE$(PACKET$, " ")
        __MOUSE_X = VAL(X$)
        __MOUSE_Y = VAL(Y$)
        __MOUSE_BUTTONS = VAL(M$)
        __MOUSE_LMB = BITAND(__MOUSE_BUTTONS, 1)
        __MOUSE_RMB = BITSHR(BITAND(__MOUSE_BUTTONS, 2), 1)
        __MOUSE_MMB = BITSHR(BITAND(__MOUSE_BUTTONS, 4), 2)
    ENDIF
ENDPROC

DEF FNmouse_x
=__MOUSE_X

DEF FNmouse_y
=__MOUSE_Y

DEF FNmouse_lmb
=__MOUSE_LMB

DEF FNmouse_rmb
=__MOUSE_RMB

DEF FNmouse_mmb
=__MOUSE_MMB
```

**What’s happening here (in parcel terms):**

* `UDPBIND "127.0.0.1", 14502 + PID` opens **your doorstep** (unique port per process).
* `UDPWRITE "127.0.0.1", 14502 + PID, 14501, "GET"` posts a **parcel** to the mouse server’s port **14501**, marking your doorstep **14502+PID** as the return address.
* `UDPREAD$(14502 + PID)` **picks up the next parcel** that arrived for you. If none is present, it returns `""` and you carry on.
* The packet encodes `x y buttons` separated by spaces; you slice those out and update state.
* `UDPUNBIND` closes the doorstep when you’re done.

Because each UDP packet is independent, you can lose one with negligible impact-the next packet will replace it a tick later, keeping the mouse fluid.

---

### Practical Tips (UDP)

* **Bind both sides**: have a predictable return address and a consistent inbox.
* **Keep messages small**: fit comfortably within your UDP payload limit to avoid IP fragmentation.
* **Design for loss**: treat every packet as optional; never rely on “the last one must have arrived”.
* **Use the labels**: read `UDPLASTIP$` / `UDPLASTSOURCEPORT` **immediately after** `UDPREAD$` if you plan to reply.

---

Right - good catch. For new users, we need to introduce the idea from first principles, **without starting with “DNS”**. Here’s a plain-English version that leads into the concept and only then names it:

---

### Names and Addresses

Every computer on the Internet has a unique **address number**, called an **IP address**. It looks like this:

```
93.184.216.34
```

These numbers are what sockets really use. But numbers are hard to remember. People prefer names, like:

```
example.org
```

There is a worldwide system that matches names to numbers, just like your phone’s contact list matches a friend’s name to their phone number. On the Internet this system is called the **Domain Name System**, usually shortened to **DNS**.

* When you type a name, DNS looks up the matching number.
* Once you have the number, you can connect your socket to it.
* The result is stored in a local cache, so asking for the same name again is instant.

---

### The DNS$ Function

Retro Rocket BASIC makes this simple with a single function:

```basic
ip$ = DNS$("example.org")
```

* Returns the IP address as a string, e.g. `"93.184.216.34"`.
* Returns `""` if the name cannot be looked up.
* Uses caching automatically; you don’t need to worry about retries or speed.

---

### Quick Examples

#### TCP Client

```basic
' Connect to a web server
sock = CONNECT("93.184.216.34", 80)
SOCKWRITE sock, "GET / HTTP/1.0" + CHR$(13) + CHR$(10) + CHR$(13) + CHR$(10)
SOCKFLUSH sock

' Read the response line
SOCKREAD sock, line$
PRINT line$

SOCKCLOSE sock
```

---

#### TCP Server

```basic
' Listen on port 8080
server = SOCKLISTEN("0.0.0.0", 8080, 5)
PRINT "Server running on port 8080"

REPEAT
    client = SOCKACCEPT(server)
    IF client >= 0 THEN
        SOCKWRITE client, "Hello from Retro Rocket!"
        SOCKFLUSH client
        SOCKCLOSE client
    ENDIF
UNTIL INKEY$ <> ""

SOCKCLOSE server
```

---

#### UDP Example

```basic
' Bind to port 5001
udp = UDPBIND("0.0.0.0", 5001)

' Send a packet to localhost:5001
UDPSEND udp, "127.0.0.1", 5001, "Ping"

' Read the next packet from the queue
msg$ = UDPREAD$(5001)
IF msg$ <> "" THEN
    PRINT "Got packet: "; msg$
    PRINT "From: "; UDPLASTIP$; " Port: "; UDPLASTSOURCEPORT
ENDIF

SOCKCLOSE udp
```

#### Connecting by Name

```basic
ip$ = DNS$("example.org")
IF ip$ <> "" THEN
    sock = CONNECT(ip$, 80)
    SOCKWRITE sock, "GET / HTTP/1.0" + CHR$(13) + CHR$(10) + CHR$(13) + CHR$(10)
    SOCKFLUSH sock
    SOCKREAD sock, reply$
    PRINT reply$
    SOCKCLOSE sock
ELSE
    PRINT "Could not look up that name"
ENDIF
```

---

### Socket Keywords

* \ref CONNECT
* \ref DNS "DNS$"
* \ref SOCKLISTEN
* \ref SOCKACCEPT
* \ref SOCKWRITE
* \ref SOCKFLUSH
* \ref SOCKREAD
* \ref SOCKCLOSE
* \ref UDPBIND
* \ref UDPSEND
* \ref UDPWRITE
* \ref UDPREAD "UDPREAD$""
* \ref UDPLASTIP "UDPLASTIP$"
* \ref UDPLASTSOURCEPORT
* \ref UDPUNBIND
