# TCP/UDP App

This project demonstrates a client-server system utilizing both TCP and UDP protocols for communication between clients and a server. The main purpose is to simulate a publish-subscribe model where clients can subscribe to topics and receive messages in real-time. The project also includes the ability to store and forward messages for clients that are temporarily disconnected, ensuring they receive all missed messages upon reconnection.

Key features include:

- **Asynchronous I/O using `poll()`**: Allows non-blocking communication with multiple clients.
- **Subscription management**: Clients can subscribe and unsubscribe to topics, receiving relevant messages.
- **Store & Forward mechanism**: Offline clients receive missed messages once they reconnect if they opted for Store & Forward.
- **TCP/UDP message handling**: Messages are encapsulated in UDP format, and the server forwards them to TCP clients subscribed to the relevant topics.

## Files

- **server.c**: Manages TCP/UDP clients, establishes connections, processes subscriptions, and forwards messages to active clients. Stores messages for offline clients if Store & Forward is enabled.
- **subscriber.c**: Acts as the client. Connects to the server, subscribes to topics, and receives messages. Can send commands to subscribe or unsubscribe from topics.
- **helper.h**: Contains definitions for the client structures and message formats for TCP and UDP.
- **queue.c/list.c**: Implements a simple queue using a linked list to store messages for clients with Store & Forward enabled.

This project is designed to provide an example of how communication protocols can be efficiently managed in a networked system, especially in environments requiring real-time data distribution and client disconnection handling.
