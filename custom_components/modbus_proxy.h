#include "esphome.h"
#include <WiFi.h>
#include <AsyncTCP.h>

class ModbusProxy : public Component {
 public:
  void setup() override;
  void loop() override;

 private:
  AsyncServer *server;
  AsyncClient *client;
  IPAddress modbus_server_ip;
  uint16_t modbus_server_port;
  void handleClient(AsyncClient *client);
};
