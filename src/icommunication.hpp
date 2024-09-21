#pragma once

#include <forward_list>

constexpr char g_stop_msg = 'S';

class ICommunication {
public:
  virtual int setup() = 0;
  virtual std::forward_list<char> read() = 0;
  virtual ~ICommunication() = default;
};

