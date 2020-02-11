#include <iostream>

#include "smmServer.hpp"

void alpha(httpMessage message,
           void* data) {
  std::cout << "callback id: " << message.getHttpVariable("callback") << std::endl;
  message.replyHttpOk();
}

void beta(httpMessage message,
          void* data) {
  std::cout << "beta" << std::endl;
  message.replyHttpContent("text/plain", "beta");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char** argv) {
  std::string httpPort = "8000";
  std::string rootPath = "./web_root";

  smmServer server(httpPort, rootPath, NULL);
  server.addPostCallback("alpha", &alpha);
  server.addGetCallback ("beta",  &beta );
  server.launch();

  while(server.isRunning()) {}
  
  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
