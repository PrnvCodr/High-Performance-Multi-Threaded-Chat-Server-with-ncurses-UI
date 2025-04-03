#include "sockutil.h"
#include <arpa/inet.h>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <ncurses.h>

#define NUM 6
#define MAX_LEN 512
#define CLIENT_SIZE sizeof(clients)
#define INPUT_HEIGHT 3
#define MESSAGE_START 1
#define USERNAME_MAX 32

uint16_t PORT_NO;
int CLIENT_SOCKET;
bool EXIT_FLAG = false;
std::thread t_send;
std::thread t_recv;
std::vector<std::string> colors = {"\033[31m", "\033[32m", "\033[33m",
                                   "\033[34m", "\033[35m", "\033[36m"};

// Ncurses windows
WINDOW *message_win = nullptr;
WINDOW *input_win = nullptr;
int max_y = 0, max_x = 0;
int message_height = 0;
std::mutex screen_mutex;
std::string username;

void setup_ncurses();
void cleanup_ncurses();
void draw_borders();
void SEND_MESSAGE(int CLIENT_SOCKET);
void RECV_MESSAGE(int CLIENT_SOCKET);
void add_message(const std::string &message);
void CATCH_CTRL_C(int signal);

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " <server_ip> <port>" << std::endl;
    return -1;
  }

  PORT_NO = atoi(argv[2]);

  CLIENT_SOCKET = socket(AF_INET, SOCK_STREAM, 0);
  if (CLIENT_SOCKET < 0)
  {
    perror("Socket creation failed");
    exit(-1);
  }

  struct sockaddr_in client_addr;
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(PORT_NO);

  if (inet_pton(AF_INET, argv[1], &client_addr.sin_addr) <= 0)
  {
    perror("Invalid address/ Address not supported");
    exit(-1);
  }

  if (connect(CLIENT_SOCKET, (struct sockaddr *)&client_addr,
              sizeof(client_addr)) < 0)
  {
    perror("Connection error");
    exit(-1);
  }

  signal(SIGINT, CATCH_CTRL_C);

  // Get username before initializing ncurses
  char name[MAX_LEN];
  std::cout << "Enter your name: ";
  std::cin.getline(name, MAX_LEN);
  send(CLIENT_SOCKET, name, sizeof(name), 0);
  username = name;

  // Setup ncurses
  setup_ncurses();
  add_message("Welcome to the chat-room, " + std::string(name) + "!");

  std::thread t1(SEND_MESSAGE, CLIENT_SOCKET);
  std::thread t2(RECV_MESSAGE, CLIENT_SOCKET);

  t_send = std::move(t1);
  t_recv = std::move(t2);

  if (t_send.joinable())
  {
    t_send.join();
  }
  if (t_recv.joinable())
  {
    t_recv.join();
  }

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
  curs_set(1); // Show cursor

  // Get screen dimensions
  getmaxyx(stdscr, max_y, max_x);
  message_height = max_y - INPUT_HEIGHT - 1;

  // Create windows
  message_win = newwin(message_height, max_x, 0, 0);
  input_win = newwin(INPUT_HEIGHT, max_x, message_height, 0);
  scrollok(message_win, TRUE);

  // Draw borders
  draw_borders();

  // Display prompt in input window
  wmove(input_win, 1, 1);
  wprintw(input_win, "You: ");
  wrefresh(input_win);
}

void draw_borders()
{
  // Draw border for message window
  box(message_win, 0, 0);
  wrefresh(message_win);

  // Draw border for input window
  box(input_win, 0, 0);
  wrefresh(input_win);
}

void cleanup_ncurses()
{
  if (message_win)
    delwin(message_win);
  if (input_win)
    delwin(input_win);
  endwin();
}

void add_message(const std::string &message)
{
  std::lock_guard<std::mutex> lock(screen_mutex);

  // Add message to message window
  wmove(message_win, getcury(message_win) + 1, 1);
  wprintw(message_win, "%s", message.c_str());

  // Refresh windows
  wrefresh(message_win);
  wrefresh(input_win);
}

void SEND_MESSAGE(int CLIENT_SOCKET)
{
  char msg[MAX_LEN];
  int cursor_x = 6; // Start after "You: "
  int input_y = 1;

  while (true)
  {
    wmove(input_win, input_y, cursor_x);
    wrefresh(input_win);

    int ch = wgetch(input_win);

    if (ch == '\n')
    {
      // Get the message from the input window
      msg[cursor_x - 6] = '\0';

      // Send message
      send(CLIENT_SOCKET, msg, sizeof(msg), 0);

      // Check if exit command
      if (strcmp(msg, "#exit") == 0)
      {
        EXIT_FLAG = true;
        t_recv.detach();
        close(CLIENT_SOCKET);
        return;
      }

      // Add message to own message window
      std::string own_msg = "You: " + std::string(msg);
      add_message(own_msg);

      // Clear input line
      wmove(input_win, input_y, 6);
      wclrtoeol(input_win);
      box(input_win, 0, 0);
      wrefresh(input_win);

      cursor_x = 6;
      memset(msg, 0, sizeof(msg));
    }
    else if (ch == KEY_BACKSPACE || ch == 127)
    {
      if (cursor_x > 6)
      {
        cursor_x--;
        wmove(input_win, input_y, cursor_x);
        waddch(input_win, ' ');
        wmove(input_win, input_y, cursor_x);
        wrefresh(input_win);
      }
    }
    else if (cursor_x < MAX_LEN + 5 && isprint(ch))
    {
      msg[cursor_x - 6] = ch;
      waddch(input_win, ch);
      cursor_x++;
    }
  }
}

void RECV_MESSAGE(int CLIENT_SOCKET)
{
  while (true)
  {
    if (EXIT_FLAG)
    {
      return;
    }

    char name[MAX_LEN];
    char msg[MAX_LEN];
    int rand;

    int bytes_recv = recv(CLIENT_SOCKET, name, sizeof(name), 0);
    if (bytes_recv <= 0)
      continue;

    recv(CLIENT_SOCKET, &rand, sizeof(rand), 0);
    recv(CLIENT_SOCKET, msg, sizeof(msg), 0);

    std::string message = std::string(name) + ": " + std::string(msg);
    add_message(message);
  }
}

void CATCH_CTRL_C(int signal)
{
  char terminate[MAX_LEN] = "#exit";
  send(CLIENT_SOCKET, terminate, sizeof(terminate), 0);
  EXIT_FLAG = true;
  t_send.detach();
  t_recv.detach();
  close(CLIENT_SOCKET);
  cleanup_ncurses();
  exit(signal);
}