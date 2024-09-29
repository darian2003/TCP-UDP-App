# TCP/UDP App

A client-server system implementing TCP and UDP communication with support for client subscriptions and message forwarding. Key features include:

- Asynchronous I/O using `poll()`
- Subscription management (`subscribe`/`unsubscribe`)
- Store & Forward for offline clients
- TCP/UDP message parsing and forwarding

The server listens for connections, manages subscriptions, and forwards messages. Subscribers can join, leave topics, and receive messages asynchronously.

## Files

- **server.c**: Manages TCP/UDP clients, connections, and message distribution.
- **subscriber.c**: Connects to server, handles message reception, and user commands.
- **helper.h**: Defines structures for TCP/UDP clients and messages.
- **queue.c/list.c**: Implements a queue for storing messages using a linked list.
