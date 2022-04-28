#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <vector>

#include "service.hpp"

static bool isClientInitialized = false;
static int sock = 0, valread;
static struct sockaddr_in serv_addr;
static const int maxBufSize = 50;
static uint8_t buffer[maxBufSize] = {0};

void parseModbusPacket();

int initTcpClientWorker() {
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary
  // form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }
  return 0;
}

int tcpClientWorker(int argc, char const* argv[], std::vector<uint8_t>& data) {
  if (!isClientInitialized) {
    if (initTcpClientWorker() >= 0) {
        isClientInitialized = true;
    };
  }

  const int maxBbufSize = 50;

  // Нужен таймаут ...
  valread = read(sock, buffer, 1024);
  printByteArray((uint8_t*)buffer);

//   data.

  // От сервера пришло сообщение.
  // Парсим ...

  // Отправяем ответ на запрос
  int messageLength = data.size();
  char response[100] = "response...\n";
  send(sock, response, strlen(response), 0);

  return 0;
}




int tcpClientWorkerRead(std::vector<uint8_t>& data) {
  if (!isClientInitialized) {
    if (initTcpClientWorker() >= 0) {
        isClientInitialized = true;
    };
  }

  if (isClientInitialized) {
    // Нужен таймаут ...
    valread = read(sock, buffer, maxBufSize);
    printByteArray((uint8_t*)buffer);

    if (valread > maxBufSize) {
      return -1;
    }
    
    if (valread > 0) {
      data.assign(buffer, buffer + valread);
    }
    

    return valread;
  } else {
    return -1;
  }
  
}


int tcpClientWorkerWrite(std::vector<uint8_t>& data) {
  if (!isClientInitialized) {
    if (initTcpClientWorker() >= 0) {
        isClientInitialized = true;
    };
  }

  // Отправяем ответ на запрос
  char *message = (char *)data.data();
  int messageLength = data.size();

  send(sock, message, messageLength, 0);

  return 0;
}
