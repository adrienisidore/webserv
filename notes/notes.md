
## TCP / IP

-> Internet protocol suite

Mulitple layers, from closer to hardware to closer to user:


1. **Link layer** (Ethernet, Wi‑Fi, PPP…)   _"Local roads"_

    - Scope: one local hop.
    - Identifiers: MAC addresses.
    - Unit: frame.
    - Job: move bits across a single link and deliver to the next device without traversing a router

2. **Internet layer** (IP: IPv4/IPv6)   _"Rules of the road, adresses, and signs"_

    - Scope: end‑to‑end across networks.
    - Identifiers: IP addresses, subnets, routes.
    - Unit: packet.
    - Job: get a packet from host A to host B across many links via routing. Emerges from many routers making local decisions

3. **Transport layer** (TCP, UDP)   _"The delivery service features"_ 

    - Scope: end‑to‑end between processes.
    - Identifiers: ports, plus connection state (for TCP)
    - Unit: segment (TCP) / datagram (UDP)
    - Job: multiplex apps (ports), add reliability/ordering/congestion control (TCP) or lightweight best‑effort (UDP). Ex: three-way handshake.

4. **Application layer** (HTTP(S), DNS, SMTP, SSH, etc.)    _"What is delivered"_

    - Scope: end‑to‑end app semantics.
    - Identifiers: URLs, hostnames, methods, headers, payload formats.
    - Unit: messages (requests/responses).
    - Job: define what the data means and how apps interact.


The app (HTTP) → hands data to TCP (ports, reliability) → hands to IP (addresses, routing) → hands to Link (frames on local medium)

## Port

**Port range**:   
    0–1023: well-known/system
    1024–49151: registered
    49152–65535: dynamic/ephemeral

Clients often connect to ephemeral ports assigned only for the duration of the transaction

HTTP uses port 80 and Telnet uses port 23

## Sockets

**socket**: IP adress family + Type of packets (sream , datagram) + Protocol used
-> On Unix a socket is a **file descriptor**

2 types of sockets:

- listening sockets (server side): bind(), listen()
- connected sockets (client or server): connect(), accept()

### Server Side (TCP):

1. socket(): creates a socket
2. bind(): choose local IP/port for the service. This is where client should connect
3. listen(): mark it as passive / listening socket
4. accept(): take the next pending connection from the listen queue and return a new connected socket (original stays in LISTEN state !)
5. read / write on the connected socket -> close()

poll() / epoll() (more powerful and linux specific): 

### Client Side (TCP)

1. socket()
2. connect(): opens a connection to server IP/port. Triggers the *3-way handshake*
3. read/write -> close()

## HTTP - Hypertext Tranfer Protocol

Application layer protocol that allows apps to communicate data. Based on TCP/IP
The server is a process

HTTP is designed to permit intermediate network elements: 
- **web cache** servers: save previously accessed resources and reuses them to improve response time
- **proxy** servers: control the complexity of the request, load balancing, privacy, security...

### Requests

#### Syntax:

```
GET /images/logo.png HTTP/1.1           # request line
Host: wwww.example.com
Accept-Language: en                     # request header fields (at least 1)
```

#### Methods

- **GET**: retreive data without making changes. can be represented by a URL alone -> cacheable-by-default

- **HEAD**: like GET but without the response body, just header. ex: To check if a resource is reachable or metadata

- **POST**: submissions or operations that change the server state. Usually can't be represented by URL alone -> non-cacheable-by-default. ex: posting a message, completing a transaction. Seding to "controller" URL.

- **PUT**: like POST but the client specifies the target location on the server

- **DELETE**


