#pragma once

constexpr char g_stop_msg = 'S';

class ICommunication {
public:
  virtual char read() = 0;
  virtual ~ICommunication() = default;
};

