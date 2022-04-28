/**
 * @file _exponenta.cpp
 * @author Ruslan Sadekov
 * @brief Требуется разработать небольшую программу под Linux,
 *        которая имеет интерфейс командной строки.
 *        Эта программа должна демонстрировать работу с Modbus TCP.
 *        Требуется запустить один экземпляр программы как сервер,
 *        другой - как клиент и показать,
 *        что взаимодействие между ними работает.
 *        В качестве результата надо показать
 *        исходный код программы и Makefile для сборки.

 * @version 0.1
 * @date 2022-04-23
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <thread>

#include "MB/TCP/connection.hpp"
#include "MB/TCP/server.hpp"
#include "MB/modbusException.hpp"
#include "MB/modbusRequest.hpp"
#include "MB/modbusResponse.hpp"
#include "service.hpp"

using namespace std;

void createRequest(MB::ModbusRequest &request);
void getResponse(MB::ModbusResponse &response);

int tcpServerWorker(int argc, char const *argv[], std::vector<uint8_t> &data);
int tcpClientWorker(int argc, char const *argv[], std::vector<uint8_t> &data);
int tcpClientWorkerRead(std::vector<uint8_t>& data);
int tcpClientWorkerWrite(std::vector<uint8_t>& data);

vector<uint8_t> rawedMasterMessage(1024, 0);
vector<uint8_t> rawedSlaveMessage(1024, 0);

MB::utils::MBFunctionCode fcode;
uint16_t address, range, slaveId;

// ----------------------------------------------------------------
/**
 * @brief Интерфейс пользователя (MASTER)
 *
 * @param run
 * @return int
 */
int masterUserInterface(bool &run) {
  cout << "\n############################################# \n";
  cout << "press {q} - to Quit\n";
  cout << "press {z} - go back\n";
  cout << "############################################# \n";

  static int res = 0;
  static enum class menuLevel_e {
    selectRegister,
    readRegister,
    writeRegister,
    selectRange,

  } menuLevel = menuLevel_e::selectRegister;

  if ((menuLevel >= menuLevel_e::selectRegister) &&
      (menuLevel <= menuLevel_e::writeRegister)) {
    int rw;
    if (menuLevel == menuLevel_e::selectRegister) {
      cout << "What you want to do - {r}ead or {w}rite :";
      rw = getchar();
      while (getchar() != '\n')
        ;
      if (rw == 'r') {
        menuLevel = menuLevel_e::readRegister;
      } else if (rw == 'w') {
        menuLevel = menuLevel_e::writeRegister;
      }
    }
    switch (menuLevel) {
      case menuLevel_e::readRegister:
        cout << "Select register type from list: \n";
        cout << "1 - ReadDiscreteOutputCoils \n";
        cout << "2 - ReadDiscreteInputContacts \n";
        cout << "3 - ReadAnalogOutputHoldingRegisters \n";
        cout << "4 - ReadAnalogInputRegisters \n";
        break;

      case menuLevel_e::writeRegister:
        cout << "Select register type from list: \n";
        cout << "5 - WriteSingleDiscreteOutputCoil \n";
        cout << "6 - WriteSingleAnalogOutputRegister \n";
        cout << "7 - WriteMultipleDiscreteOutputCoils \n";
        cout << "8 - WriteMultipleAnalogOutputHoldingRegisters \n";
        break;

      default:
        break;
    }

    res = getchar();
    while (getchar() != '\n')
      ;

    switch (res) {
      case 'q':
        run = false;
        break;
      case '1':
        fcode = MB::utils::ReadDiscreteOutputCoils;
        break;
      case '2':
        fcode = MB::utils::ReadDiscreteInputContacts;
        break;
      case '3':
        fcode = MB::utils::ReadAnalogOutputHoldingRegisters;
        break;
      case '4':
        fcode = MB::utils::ReadAnalogInputRegisters;
        break;
      case '5':
        fcode = MB::utils::WriteSingleDiscreteOutputCoil;
        break;
      case '6':
        fcode = MB::utils::WriteSingleAnalogOutputRegister;
        break;
      case '7':
        fcode = MB::utils::WriteMultipleDiscreteOutputCoils;
        break;
      case '8':
        fcode = MB::utils::WriteMultipleAnalogOutputHoldingRegisters;
        break;
      default:
        fcode = MB::utils::Undefined;
        break;
    }
  }

  // Выбор диапазона адресов и начального адреса регистра
  if (fcode != MB::utils::Undefined) {
    menuLevel = menuLevel_e::selectRange;
    int c;
    cout << "  Enter slaveId: " << ends;
    slaveId = 0;
    while ((c = getchar()) != '\n') {
      if (c >= 0x30 && c <= 0x39) {
        slaveId = slaveId * 10 + (c - 0x30);
      } else if (c == 'q') {
        return -1;
      } else if (c == 'z') {
        menuLevel = menuLevel_e::selectRegister;
        return 0;
      }
    };

    cout << "  Enter start address: " << ends;
    address = 0;
    while ((c = getchar()) != '\n') {
      if (c >= 0x30 && c <= 0x39) {
        address = address * 10 + (c - 0x30);
      } else if (c == 'q') {
        return -1;
      } else if (c == 'z') {
        menuLevel = menuLevel_e::selectRegister;
        return 0;
      }
    };

    cout << "  Enter range: " << ends;
    range = 0;
    while ((c = getchar()) != '\n') {
      if (c >= 0x30 && c <= 0x39) {
        range = range * 10 + (c - 0x30);
      } else if (c == 'q') {
        return -1;
      } else if (c == 'z') {
        menuLevel = menuLevel_e::selectRegister;
        return 0;
      }
    };

    cout << address << ":" << range << "\n";

  } else {
    return 0;
  }

  return 1;
}

// ----------------------------------------------------------------
/**
 * @brief Интерфейс пользователя (SLAVE)
 *
 * @param run
 * @return int
 */
int slaveUserInterface(bool &run) {
  cout << "\n############################################# \n";
  cout << "press {q} - to Quit\n";
  cout << "press {z} - go back\n";
  cout << "############################################# \n";

  static int res = 0;
  static bool isConfigurated = false;

  if (isConfigurated == false) {
    int c;
    cout << "  Enter slaveId: " << ends;
    slaveId = 0;
    while ((c = getchar()) != '\n') {
      if (c >= 0x30 && c <= 0x39) {
        slaveId = slaveId * 10 + (c - 0x30);
      } else if (c == 'q') {
        run = false;
        return -1;
      } else if (c == 'z') {
        run = false;
        return 0;
      }
    };
    isConfigurated = true;
  }
  return 1;
}

// ----------------------------------------------------------------
void startModbusServer() {
  bool run = true;
  std::cout << "master...\n";
  MB::ModbusRequest *req;
  while (run) {
    int uiRes = masterUserInterface(run);
    range = 1; /// ----------------------------------------------<<<<
    if (uiRes == -1) {
      run = false;
      break;
    } else if (uiRes == 1) {
      std::vector<MB::ModbusCell> data;
      switch (fcode) {
        case MB::utils::ReadDiscreteOutputCoils:
        case MB::utils::ReadDiscreteInputContacts:
        // std::vector<MB::ModbusCell::coil> data = {0b0, 0b1, 0b0, 0b1};
        req = new MB::ModbusRequest(slaveId, fcode, address, range);
        createRequest(*req);
        break;
        case MB::utils::ReadAnalogInputRegisters:
        case MB::utils::ReadAnalogOutputHoldingRegisters:
        // std::vector<MB::ModbusCell::coil> data = {0xffaa, 0xf1a2, 0x3fa1, 0x4f5a, 0xf6ae};
        req = new MB::ModbusRequest(slaveId, fcode, address, range);
        createRequest(*req);
        break;

        case MB::utils::WriteSingleDiscreteOutputCoil:
        case MB::utils::WriteMultipleDiscreteOutputCoils: {
          data.push_back(MB::ModbusCell::initCoil(1));
          data.push_back(MB::ModbusCell::initCoil(0));
          data.push_back(MB::ModbusCell::initCoil(1));
          data.push_back(MB::ModbusCell::initCoil(1));
          req = new MB::ModbusRequest(slaveId, fcode, address, range, data);
          createRequest(*req);
        
        }
        break;
        case MB::utils::WriteSingleAnalogOutputRegister:
        case MB::utils::WriteMultipleAnalogOutputHoldingRegisters: {
          data.push_back(MB::ModbusCell::initReg(0xa2f4));
          data.push_back(MB::ModbusCell::initReg(0xb4f2));
          data.push_back(MB::ModbusCell::initReg(0xe09f));
          data.push_back(MB::ModbusCell::initReg(0x3fcc));
          req = new MB::ModbusRequest(slaveId, fcode, address, range, data);
          createRequest(*req);
        }
        break;
      }
      
      tcpServerWorker(0, nullptr, rawedMasterMessage);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}



// ----------------------------------------------------------------
void startModbusClient() {
  bool run = true;
  while (run) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (slaveUserInterface(run) == 1) {
      std::vector<uint8_t> masterData;
      int resp = tcpClientWorkerRead(masterData);

      if (resp > 0) {
        while (masterData.size() > resp+1) {
          masterData.pop_back();
        };
        MB::ModbusRequest request(masterData, true);
        // vector <uint8_t> values = ....
        // getValues() random....
        MB::ModbusResponse response (slaveId, request.functionCode(), 
        request.registerAddress(), request.numberOfRegisters(), request.registerValues());
        // MB::ModbusResponse response(masterData, true);
        getResponse(response);
        tcpClientWorkerWrite(rawedSlaveMessage);
      }
      
    }
  }
  return;
}


void createRequest(MB::ModbusRequest &request) {
  // Create simple request
  std::cout << "Stringed Request: " << request.toString() << std::endl;

  std::cout << "Raw request:" << std::endl;

  // Get raw represenatation for request

  std::vector<uint8_t> &rawed = rawedMasterMessage;
  rawedMasterMessage = request.toRaw();

  // Method for showing byte
  auto showByte = [](const uint8_t &byte) {
    std::cout << " 0x" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(byte);
  };

  // Show all bytes
  std::for_each(rawed.begin(), rawed.end(), showByte);
  std::cout << std::endl;

  // Create CRC and pointer to its bytes
  uint16_t CRC = MB::utils::calculateCRC(rawed);
  auto CRCptr = reinterpret_cast<uint8_t *>(&CRC);

  // Show byted CRC for request
  std::cout << "CRC for the above code: ";
  std::for_each(CRCptr, CRCptr + 2, showByte);
  std::cout << std::endl;

  auto request1 = MB::ModbusRequest::fromRaw(rawed);
  std::cout << "Stringed Request 1 after rawed: " << request1.toString()
            << std::endl;

  // Add CRC to the end of raw request so that it can be loaded with CRC check
  rawed.insert(rawed.end(), CRCptr, CRCptr + 2);

  auto request2 =
      MB::ModbusRequest::fromRawCRC(rawed);  // Throws on invalid CRC
  std::cout << "Stringed Request 2 after rawed: " << request2.toString()
            << std::endl;
}



// -----------------------------------------------------------------------------

// void getResponse(MB::ModbusResponse &response) {
void getResponse(MB::ModbusResponse &response) {
  std::cout << "Stringed Response: " << response.toString() << std::endl;
  std::cout << "Raw response:" << std::endl;

  // Get raw represenatation for response
  std::vector<uint8_t> &rawed = rawedSlaveMessage;
  rawedSlaveMessage = response.toRaw();

  // Method for showing byte
  auto showByte = [](const uint8_t &byte) {
    std::cout << " 0x" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(byte);
  };

  // Show all bytes
  std::for_each(rawed.begin(), rawed.end(), showByte);
  std::cout << std::endl;

  // Create CRC and pointer to its bytes
  uint16_t CRC = MB::utils::calculateCRC(rawed);
  auto CRCptr = reinterpret_cast<uint8_t *>(&CRC);

  // Show byted CRC for request
  std::cout << "CRC for the above code: ";
  std::for_each(CRCptr, CRCptr + 2, showByte);
  std::cout << std::endl;

  auto request1 = MB::ModbusResponse::fromRaw(rawed);
  std::cout << "Stringed ModbusResponse 1 after rawed: " << request1.toString()
            << std::endl;

  // Add CRC to the end of raw request so that it can be loaded with CRC check
  rawed.insert(rawed.end(), CRCptr, CRCptr + 2);

  auto request2 =
      MB::ModbusResponse::fromRawCRC(rawed);  // Throws on invalid CRC
  std::cout << "Stringed ModbusResponse 2 after rawed: " << request2.toString()
            << std::endl;
}
