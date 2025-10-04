
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
**ADRI'S VERSION** : download a ressource.

- **PUT**: like POST but the client specifies the target location on the server.
**ADRI'S VERSION** : modify or replace a ressource at a precise location.


- **DELETE**


#### Authorized functions

**execve** : executes a process and replace the current image's process.

**access** : test if a process has a certain permission.

**pipe** : one-directionial data transfer pipe.

**socketpair** : bi-directional data transfer pipe.

**socket** : create a socket.

**accept** : accept the connection of one client.

**connect** : connect a client to a server.

**bind** : bind a socket to an IP/Port couple. IP/Port can now receive/send data. 

**listen** : makes a socket look for a new client connection.

**send** : send some data.

**recv** : wait some data to be received.

**setsockopt** : configure a socket behaviour.

**getsockname** : to know which IP/Port a socket is connected to.


**errno** : get the last error code.

**strerror** : convert an errno to a message in english.

**gai_strerror** : convert an error code coming from getaddrinfo(), getnameinfo() to a message in english.


**htonl, htons** : translate the value of an IP address (htonl) or Port (htons) to a network order, before sending it to another system

**ntohl, ntohs** : convert a network order into an IP address (ntohl) or Port (ntohs), after receiving it from another system

**dup** : duplicate the fd.

**dup2** : redirection of a dataflow from a fd to another.

**close** : close an fd (socket, pipe ...).

**fcntl** : configurate an fd (permissions etc...)

**read** : get the bytes of a file.

**write** : write some bytes in a file.

**stat** : a structure containing the properties of a file.

**open** : open a file.

**fork** : create a child process.

**waitpid** : wait for a process (PID) to terminate.

**poll, epoll (epoll_create, epoll_ctl, epoll_wait), kqueue (kqueue, kevent)** : monitor the sockets.

**select** : similar to poll() but less powerful.

**getaddrinfo** : returns every IP address that respect certain criterias (IPv4, compatible with TCP ...)

**freeaddrinfo** : free the linked list of IP address that respect certain criterias.

**getprotobyname**: return "struct protoent", that contains the name, the alias and the ID of a protocole (TCP, UDP...)

**chdir** : changes the current directory.

**opendir** : open a designated repository.

**readdir** : return a pointer of a repository or a file.

**closedir** : close a designated repository.

**kill(pid, sig)** : send the "sig" signal to a process, identified by it's PID.

**signal** : define a function to manage the "sig" signal.








