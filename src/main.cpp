#include <iostream>

#include "service.hpp"
#include "stdint.h"
#include "stdio.h"
#include "string.h"

void startModbusServer(void);
void startModbusClient(void);

int main(int argc, char **argv) {
  int answer = 0;
  for (int i = 0; i < argc; ++i) {
    bool isServer = (strcmp(argv[i], "-s") == 0) ||
                    (strcmp(argv[i], "-S") == 0) ||
                    (strcmp(argv[i],  "s") == 0) ||
                    (strcmp(argv[i],  "S") == 0) ;

    bool isClient = (strcmp(argv[i], "-c") == 0) ||
                    (strcmp(argv[i], "-C") == 0) ||
                    (strcmp(argv[i],  "c") == 0) ||
                    (strcmp(argv[i],  "C") == 0) ;
    if (isServer) {
      answer = 's';
      break;
    }
    if (isClient) {
      answer = 'c';
      break;
    }
  }

  if (answer == 0) {
    std::cout << "Select mode:\n s - server, c - client\n";
    answer = getchar();
    while (getchar() != '\n');
  }
  
  switch (answer) {
    case 's':
      std::cout << "Starting modbus server...\n";
      startModbusServer();
      break;

    case 'c':
      std::cout << "Starting modbus client...\n";
      startModbusClient();
      break;

    default:
      break;
  }

  std::cout << "Terminating process" << std::endl;

  return 0;
}
