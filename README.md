# High-Performance Multi-Threaded Chat Server with ncurses UI

A highly optimized, low-latency multi-threaded chat server built in C++ with `ncurses` for a rich terminal-based UI. This project demonstrates efficient networking, multi-threading, and real-time communication.

## Features

- **Multi-threaded Server:** Handles multiple clients concurrently with threading.
- **Efficient Networking:** Uses socket programming for real-time message exchange.
- **Ncurses UI:** Provides an interactive chat interface in the terminal.
- **Low Latency Optimizations:** Designed to minimize message delay.
- **Graceful Exit Handling:** Catches signals and closes sockets safely.
- **Customizable Colors & UI Elements.**

## Screenshot

![Chat Server Example](Screenshot%202025-04-03%20215233.png)

## Installation & Usage

### 1. Clone the Repository
```sh
git clone https://github.com/PrnvCodr/High-Performance-Multi-Threaded-Chat-Server-with-ncurses-UI.git
cd High-Performance-Multi-Threaded-Chat-Server-with-ncurses-UI
```

### 2. Compile the Project
```sh
g++ server.cpp sockutil.cpp -o server -lpthread

# Compile client with ncurses
g++ client.cpp sockutil.cpp -o client -lpthread -lncurses
```

### 3. Run the Server
```sh
./server 8080
```

### 4. Run the Client
```sh
./client 127.0.0.1 8080
```

## Future Improvements
- Convert this project into a **web-based chat app** using WebSockets.
- Implement **even lower latency** optimizations for ultra-fast messaging.
- Add **encryption** for secure communication.
- Support for **file sharing** between clients.

## Contributing
Contributions are welcome! Feel free to open issues or submit pull requests.


