#pragma once

#include "OpenBCI_Wifi_Definitions.h"
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <Print.h>

template<size_t BUFFER_SIZE = 1440>
class WiFiClientPrint : public Print
{
  public:
    WiFiClientPrint()
      : _clientTCP(),
        _length(0),
        _mode(0)
    {
    }
    WiFiClientPrint(WiFiClient client)
      : _clientTCP(client),
        _length(0),
        _mode(0)
    {
    }
    WiFiClientPrint(WiFiUDP client)
      : _clientUDP(client),
        _length(0),
        _mode(1)
    {
    }
    ~WiFiClientPrint()
    {
#ifdef DEBUG_ESP_PORT
      // Note: This is manual expansion of assertion macro
      if (_length != 0) {
        DEBUG_ESP_PORT.printf("\nassertion failed at " __FILE__ ":%d: " "_length == 0" "\n", __LINE__);
        // Note: abort() causes stack dump and restart of the ESP
        abort();
      }
#endif
    }

    virtual size_t write(uint8_t c) override
    {
      _buffer[_length++] = c;
      if (_length == BUFFER_SIZE) {
        flush();
      }
    }

    void flush()
    {
      if (_length != 0) {
        // for (size_t i = 0; i < _length; i++) {
        //   Serial.print((char)_buffer[i]);
        // }
        if (_mode == 0) {
          _clientTCP.write((const uint8_t*)_buffer, _length);
        } else {
          _clientUDP.write((const uint8_t*)_buffer, _length);
        }
        _length = 0;
      }
    }

    void setClient(WiFiClient client)
    {
      _mode = 0;
      _clientTCP = client;
    }

    void setClient(WiFiUDP client)
    {
      _mode = 1;
      _clientUDP = client;
    }

    void stop()
    {
      flush();
      if (_mode == 0) {
        _clientTCP.stop();
      } else {
        _clientUDP.stop();
      }
    }

  private:
    WiFiClient _clientTCP;
    WiFiUDP _clientUDP;
    uint8_t _buffer[BUFFER_SIZE];
    size_t _length;
    uint8_t _mode;
};
