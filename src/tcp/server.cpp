#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "service.hpp"

inline void throwErrorMessage(const char *__s) {
  perror(__s);
  exit(EXIT_FAILURE);
};

static bool isServerInitialized = false;
static struct sockaddr_in serv_address;
static int server_fd, new_socket, valread;
static char buffer[256] = {0};
static int addrlen;

bool tcpServerInit() {
  int opt = 1;
  addrlen = sizeof(serv_address);

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    throwErrorMessage("socket failed");
  }

  int res;

  // Forcefully attaching socket to the port 8080
  res = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt));
  if (res != 0) {
    throwErrorMessage("setsockopt");
  }

  serv_address.sin_family = AF_INET;
  serv_address.sin_addr.s_addr = INADDR_ANY;
  serv_address.sin_port = htons(PORT);

  // Forcefully attaching socket to the port 8080
  res = bind(server_fd, (struct sockaddr *)&serv_address, sizeof(serv_address));
  if (res != 0) {
    throwErrorMessage("bind failed");
  }

  if (listen(server_fd, 3) < 0) {
    throwErrorMessage("listen");
  }

  new_socket =
      accept(server_fd, (struct sockaddr *)&serv_address, (socklen_t *)&addrlen);
  if (new_socket < 0) {
    throwErrorMessage("accept");
  }
  return 0;
}



int tcpServerWorker(int argc, char const *argv[], std::vector<uint8_t> &data) {
  if (!isServerInitialized) {
    tcpServerInit();
    isServerInitialized = true;
  }
  
  char *message = (char *)data.data();
  int messageLength = data.size();
  
  // Отправляем запрос ...
  send(new_socket, message, messageLength, 0);
  printf("Request sent \n");

  // Ждём ответ
  valread = read(new_socket, buffer, messageLength);
  printf("Received responce - %s\n", buffer);
  
  return 0;
}

