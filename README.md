# High-Performance Multi-Threaded Chat Server with ncurses UI

## 🚀 Introduction
This project is a high-performance, multi-threaded chat server implemented in C++. It features an intuitive **ncurses-based UI**, making terminal-based chatting more interactive. The server is designed for **low-latency** communication and supports multiple clients efficiently.

## 🔥 Features
- **Multi-threaded architecture** for handling multiple clients concurrently
- **ncurses UI** for an improved terminal chat experience
- **Low-latency message transmission** for real-time chatting
- **Client-server model** with TCP socket programming
- **Signal handling** for safe termination
- **Efficient synchronization** using mutexes

## 🛠️ Installation & Setup
### Prerequisites
- C++ compiler (g++)
- ncurses library
- Git (for cloning)

### Steps
```sh
# Clone the repository
git clone https://github.com/PrnvCodr/High-Performance-Multi-Threaded-Chat-Server-with-ncurses-UI.git
cd High-Performance-Multi-Threaded-Chat-Server-with-ncurses-UI

# Compile the server and client
g++ -o server server.cpp sockutil.cpp -lpthread -lncurses
g++ -o client client.cpp sockutil.cpp -lpthread -lncurses

# Run the server
./server 8080

# Run the client
./client 127.0.0.1 8080
```

## 🖥️ Usage
1. Start the server on a designated port.
2. Clients can connect using the server IP and port.
3. Enjoy real-time chat with multiple users in a **low-latency** environment.

## 🔧 Optimizations & Future Plans
- **Further latency reduction** through advanced network optimizations.
- **Web-based UI** for a more modern chatting experience.
- **Integration with WebSockets** for real-time messaging.
- **Load balancing** for scalability.
- **Database integration** for chat history persistence.

## 🤝 Contributing
Contributions are welcome! Feel free to fork this project and submit a PR with your improvements.



---
💡 Built with passion for high-performance computing and real-time communication!
