#include "modbus_proxy.h"

void ModbusProxy::setup() {
  // Initialize server and start listening on a port
  server = new AsyncServer(502); // Listening on Modbus TCP default port
  server->onClient([this](void *s, AsyncClient *c) {
    this->handleClient(c);
  }, server);
  server->begin();

  // Set the Modbus server IP and port
  modbus_server_ip = IPAddress(192, 168, 1, 100); // Example IP
  modbus_server_port = 502; // Modbus TCP port
}

void ModbusProxy::loop() {
  // Nothing to do here, as the AsyncServer handles the connections
}

void ModbusProxy::handleClient(AsyncClient *client) {
  this->client = client;
  client->onData([this](void *c, AsyncClient *client, void *data, size_t len) {
    // Forward data to Modbus server
    WiFiClient modbus_client;
    if (modbus_client.connect(modbus_server_ip, modbus_server_port)) {
      modbus_client.write((uint8_t*)data, len);
      while (modbus_client.connected()) {
        while (modbus_client.available()) {
          size_t len = modbus_client.available();
          uint8_t data[len];
          modbus_client.read(data, len);
          client->write(data, len);
        }
      }
      modbus_client.stop();
    }
  });
}
