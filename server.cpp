#include "sockutil.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <errno.h>
#include <iostream>
#include <list>
#include <mutex>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <ncurses.h>

#define NUM 6
#define MAX_LEN 512
#define CLIENT_SIZE sizeof(clients)

int u_id = 0;
std::mutex client_mtx;
std::mutex cout_mtx;
std::vector<std::string> colors = {"\033[31m", "\033[32m", "\033[33m",
                                   "\033[34m", "\033[35m", "\033[36m"};
std::vector<CLIENT_INFO> clients;

// Ncurses windows
WINDOW *log_win = nullptr;
WINDOW *status_win = nullptr;
int max_y = 0, max_x = 0;
int log_height = 0;
int status_height = 8;

void setup_ncurses();
void cleanup_ncurses();
void draw_borders();
void add_log_message(const std::string &message);
void update_status_window();

void HANDLE_CLIENT(int client_socket, int id);
void BROADCASTING(std::string message, int SENDER_ID);
void END_CON(int ID);
void BROADCAST_ID_FOR_COLOR(int num, int SENDER_ID);
int GET_CLIENT_COUNT();
std::string GetClientsInfo();
void SAVE_TO_CACHE(int u_id, std::string msg);
std::string Get_old_message();

// Cache implementation with replacing key so only latest message from each client
class Cache
{
public:
  struct Node
  {
    std::string key;
    std::string value;
    Node *prev;
    Node *next;
    Node()
    {
      key = "";
      value = "";
    }
    Node(const std::string &k, const std::string &v)
    {
      key = k;
      value = v;
    }
  };
  std::unordered_map<std::string, Node *> cacheMap;
  Node *head = new Node();
  Node *tail = new Node();
  int CAPACITY;

  Cache(int cap)
  {
    CAPACITY = cap;
    head->next = tail;
    tail->prev = head;
  }

  void ADD_Node(Node *newnode);
  void Del_Node(Node *newnode);
  void PUT_DATA(const std::string &key, const std::string &value);
};

void Cache::PUT_DATA(const std::string &key, const std::string &value)
{
  if (cacheMap.find(key) != cacheMap.end())
  {
    Node *exist = cacheMap[key];
    cacheMap.erase(key);
    Del_Node(exist);
  }
  if (cacheMap.size() == CAPACITY)
  {
    cacheMap.erase(tail->prev->key);
    Del_Node(tail->prev);
  }
  ADD_Node(new Node(key, value));
  cacheMap[key] = head->next;
}

void Cache::Del_Node(Node *delnode)
{
  Node *nodeprev = delnode->prev;
  Node *nodenext = delnode->next;
  nodeprev->next = nodenext;
  nodenext->prev = nodeprev;
}

void Cache::ADD_Node(Node *newnode)
{
  Node *temp = head->next;
  newnode->next = temp;
  newnode->prev = head;
  head->next = newnode;
  temp->prev = newnode;
}

Cache cache(5);

void CATCH_CTRL_C(int signal)
{
  cleanup_ncurses();
  exit(signal);
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
    return 0;
  }

  int PORT_NO = atoi(argv[1]);

  int server_socket = createTCPipv4socket();
  if (server_socket < 0)
  {
    std::cout << "Failed to create socket" << std::endl;
    exit(-1);
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT_NO);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  bzero(&server_addr.sin_zero, 0);

  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0)
  {
    std::cout << "Failed bind" << std::endl;
    perror("Bind error: ");
    exit(-1);
  }

  if (listen(server_socket, 8) < 0)
  {
    perror("Listen error: ");
    exit(-1);
  }

  // Setup ncurses
  setup_ncurses();
  signal(SIGINT, CATCH_CTRL_C);

  add_log_message("Server is listening on port " + std::to_string(PORT_NO));
  add_log_message("Waiting for connections...");

  struct sockaddr_in client_addr;
  int client_socket;
  unsigned int len = sizeof(sockaddr_in);

  // Update status window with initial information
  update_status_window();

  while (1)
  {
    if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr,
                                &len)) < 0)
    {
      add_log_message("Accept error: " + std::string(strerror(errno)));
      continue;
    }
    u_id++;
    add_log_message("New connection from " +
                    std::string(inet_ntoa(client_addr.sin_addr)) +
                    ":" + std::to_string(ntohs(client_addr.sin_port)) +
                    " (assigned ID: " + std::to_string(u_id) + ")");

    std::thread t(HANDLE_CLIENT, client_socket, u_id);

    {
      std::lock_guard<std::mutex> guard(client_mtx);
      std::string random = "anonymous";
      clients.push_back({u_id, random, client_socket, (move(t))});
      update_status_window();
    }
  }

  for (int i = 0; i < clients.size(); i++)
  {
    if (clients[i].th.joinable())
    {
      clients[i].th.join();
    }
  }

  close(server_socket);
  cleanup_ncurses();
  return 0;
}

void setup_ncurses()
{
  // Initialize ncurses
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  start_color();
  use_default_colors();
  curs_set(0); // Hide cursor

  // Get screen dimensions
  getmaxyx(stdscr, max_y, max_x);
  log_height = max_y - status_height - 1;

  // Create windows
  log_win = newwin(log_height, max_x, 0, 0);
  status_win = newwin(status_height, max_x, log_height, 0);
  scrollok(log_win, TRUE);

  // Draw borders
  draw_borders();
}

void draw_borders()
{
  // Draw border for log window
  box(log_win, 0, 0);
  mvwprintw(log_win, 0, 2, " Server Log ");
  wrefresh(log_win);

  // Draw border for status window
  box(status_win, 0, 0);
  mvwprintw(status_win, 0, 2, " Connected Clients ");
  wrefresh(status_win);
}

void cleanup_ncurses()
{
  if (log_win)
    delwin(log_win);
  if (status_win)
    delwin(status_win);
  endwin();
}

void add_log_message(const std::string &message)
{
  std::lock_guard<std::mutex> guard(cout_mtx);

  // Add timestamp
  time_t now = time(0);
  char timestamp[20];
  strftime(timestamp, sizeof(timestamp), "%H:%M:%S", localtime(&now));

  // Add message to log window
  wmove(log_win, getcury(log_win) + 1, 1);
  wprintw(log_win, "[%s] %s", timestamp, message.c_str());

  // Refresh window
  wrefresh(log_win);
}

void update_status_window()
{
  std::lock_guard<std::mutex> guard(cout_mtx);

  // Clear status window content (not the border)
  werase(status_win);
  box(status_win, 0, 0);
  mvwprintw(status_win, 0, 2, " Connected Clients ");

  // Display client information
  int y = 1;
  if (clients.empty())
  {
    mvwprintw(status_win, y++, 1, "No clients connected");
  }
  else
  {
    for (const auto &client : clients)
    {
      mvwprintw(status_win, y++, 1, "ID: %d, Name: %s",
                client.id, client.name.c_str());
      if (y >= status_height - 1)
        break; // Prevent overflow
    }
  }

  // Refresh window
  wrefresh(status_win);
}

void SET_NAME(int id, char name[])
{
  for (int i = 0; i < clients.size(); ++i)
  {
    if (clients[i].id == id)
    {
      clients[i].name = std::string(name);
      update_status_window();
      break;
    }
  }
}

int GET_CLIENT_COUNT()
{
  return clients.size();
}

void HANDLE_CLIENT(int client_socket, int id)
{
  char name[MAX_LEN];
  char msg[MAX_LEN];
  recv(client_socket, name, sizeof(name), 0);
  SET_NAME(id, name);

  // Welcome message
  std::string WELCOME = std::string(name) + " has joined the party";
  add_log_message(WELCOME);

  BROADCASTING("NEW_CON", id);
  BROADCAST_ID_FOR_COLOR(id, id);
  BROADCASTING(WELCOME, id);

  while (true)
  {
    int bytes_recv = recv(client_socket, msg, sizeof(msg), 0);
    if (bytes_recv <= 0)
    {
      return;
    }
    if (strcmp(msg, "#exit") == 0)
    {
      // Leaving message
      std::string bye = std::string(name) + " has left";
      add_log_message(bye);

      BROADCASTING("NEW_CON", id);
      BROADCAST_ID_FOR_COLOR(id, id);
      BROADCASTING(bye, id);
      END_CON(id);
      return;
    }

    if (strcmp(msg, "#gc") == 0)
    {
      uint16_t COUNT = GET_CLIENT_COUNT();
      std::string INFO = GetClientsInfo();
      char buff[MAX_LEN] = "NEW_CON";

      send(client_socket, buff, sizeof(buff), 0);
      send(client_socket, &id, sizeof(id), 0);
      send(client_socket, INFO.c_str(), INFO.length(), 0);
      continue;
    }

    if (strcmp(msg, "#cli") == 0)
    {
      add_log_message(std::string(name) + " requested private chat");
      char target[MAX_LEN];
      recv(client_socket, target, sizeof(target), 0);

      int target_socket = -1;
      for (const auto &client : clients)
      {
        if (client.name == std::string(target))
        {
          target_socket = client.socket;
          break;
        }
      }

      if (target_socket != -1)
      {
        std::string baat = "You are now chatting separately with " + std::string(name);
        char buff[MAX_LEN] = "NEW_CON";

        send(target_socket, buff, sizeof(buff), 0);
        send(target_socket, &id, sizeof(id), 0);
        send(target_socket, baat.c_str(), baat.length(), 0);
        std::string bat = "You are now chatting separately with " + std::string(target);
        add_log_message(std::string(name) + " started private chat with " + std::string(target));
      }
      else
      {
        std::string tar_msg = "Target not found";
        send(client_socket, tar_msg.c_str(), sizeof(tar_msg), 0);
        add_log_message("Private chat target not found for " + std::string(name));
      }
    }

    if (strcmp(msg, "#getmsg") == 0)
    {
      std::string INFO_MSG = Get_old_message();
      char buff[MAX_LEN] = "NEW_CONN";
      size_t INFO_SIZE = INFO_MSG.size();
      send(client_socket, buff, sizeof(buff), 0);
      send(client_socket, &id, sizeof(id), 0);
      send(client_socket, INFO_MSG.c_str(), INFO_SIZE, 0);
      continue;
    }

    BROADCASTING(std::string(name), id);
    BROADCAST_ID_FOR_COLOR(id, id);
    SAVE_TO_CACHE(id, msg);
    BROADCASTING(std::string(msg), id);
    add_log_message(std::string(name) + ": " + std::string(msg));
  }
}

void SAVE_TO_CACHE(int id, std::string msg)
{
  cache.PUT_DATA(std::to_string(id), msg);
}

void BROADCAST_ID_FOR_COLOR(int num, int SENDER_ID)
{
  for (int i = 0; i < clients.size(); ++i)
  {
    if (clients[i].id != SENDER_ID)
    {
      send(clients[i].socket, &num, sizeof(num), 0);
    }
  }
}

void BROADCASTING(std::string message, int SENDER_ID)
{
  char msg[MAX_LEN];
  strcpy(msg, message.c_str());
  for (int i = 0; i < clients.size(); ++i)
  {
    if (clients[i].id != SENDER_ID)
    {
      send(clients[i].socket, msg, sizeof(msg), 0);
    }
  }
}

void END_CON(int ID)
{
  for (int i = 0; i < clients.size(); ++i)
  {
    if (clients[i].id == ID)
    {
      std::lock_guard<std::mutex> guard(client_mtx);
      add_log_message("Client disconnected: " + clients[i].name + " (ID: " + std::to_string(ID) + ")");
      clients[i].th.detach();
      clients.erase(clients.begin() + i);
      update_status_window();
      break;
    }
  }
}

std::string GetClientsInfo()
{
  std::string clientInfo = "Active Members:\n";

  for (const auto &client : clients)
  {
    clientInfo += "ID: " + std::to_string(client.id) + ", Name: " + client.name + "\n";
  }

  return clientInfo;
}

std::string Get_old_message()
{
  std::string oldmsg = "Last 5 messages:\n";

  for (const auto &it : cache.cacheMap)
  {
    // Append the ID to the message
    oldmsg += "ID: " + it.first + " msg: " + it.second->value + "\n";
  }
  return oldmsg;
}

std::string color(int code)
{
  return colors[code % NUM];
}