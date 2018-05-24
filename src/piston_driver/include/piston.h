#ifndef PISTON_H
#define PISTON_H

#include <sys/types.h>
#include <iostream>
#include <fstream>

#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include <ros/ros.h>

#define I2C_PISTON_MOVE 0xFE
#define I2C_PISTON_SPEED 0xAB
#define I2C_PISTON_CMD 0xEE
#define I2C_PISTON_BLANK_VALUE 0xAA

/*
* octet1 = 0xFE --> correspond à une consigne de position, suivit de la valeur de la position à atteindre
                    sur deux octets

* octet1 = 0xAB --> correspond à une consigne de vitesse, suivit de la valeur de la vitesse
                    sur deux octets

* octet1 = 0xEE --> correspond à une consigne de marche,arret,mise en butee (sur 1 octets)
* valeur 0xAA 0x00: arret
* valeur 0xAA 0x01: marche
* valeur 0xAA 0x02: mise en butee butee de sortie
* valeur 0xAA 0x03: mise en butee butee de rentree


* octet1 = 0x00 --> correspond à une demande, suivit de la valeur de la correspondance de la consigne
* valeur 0xAA 0x0A: valeur de la tension de batterie 1 sur 2 octets
* valeur 0xAA 0x0C: valeur de la tension de batterie 2 sur 2 octets
* valeur 0xAA 0x0E: valeur de la tension de batterie 3 sur 2 octets
* valeur 0xAA 0x10: valeur de la tension de batterie 4 sur 2 octets
* valeur 0xAA 0x04: valeur NB impulsions  sur 2 octets
* valeur 0xAA 0x06: valeur de la butée de sortie  sur 2 octets
* valeur 0xAA 0x08: valeur de la butée de rentrée sur 2 octets
*/

class Piston
{
public:
  Piston();
  ~Piston();

  int i2c_open();

  uint32_t set_piston_start() const;
  uint32_t set_piston_stop() const;
  uint32_t set_piston_speed(const uint16_t &speed) const;
  uint32_t set_piston_position(const uint16_t &position) const;
  uint32_t set_piston_enable(const bool &val) const;
  uint32_t set_piston_reset() const;

  const uint16_t& get_piston_position();
  const bool& get_piston_switch_out();
  const bool& get_piston_switch_in();
  const uint16_t& get_piston_state();
  const bool& get_piston_system_on();
  const bool& get_piston_motor_on();
  const bool& get_piston_enable_on();
  const uint16_t& get_piston_position_set_point();
  void update_piston_all_data();

  void write_cmd(const unsigned short &left, const unsigned short &right) const;

public:
  uint16_t m_position = 0;
  bool m_switch_out = false;
  bool m_switch_in = false;
  uint16_t m_state = 0;
  bool m_system_on = false;
  bool m_motor_on = true;
  bool m_enable_on = true;
  uint16_t m_position_set_point = 0;
  uint16_t m_motor_speed = 0;

private:
  int m_file;
  const int m_i2c_addr = 0x38;
  const char* m_i2c_periph = "/dev/i2c-1";
};


#endif // PISTON_H
