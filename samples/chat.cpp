#include "chat/server.h"
#include "chat/buffered_logging.h"

/*
 Simple chat server
 
 Build and run:
  make run FILE=samples/chat.cpp               #Uses default port 9090
  make run FILE=samples/chat.cpp ARGS="8080"   #Uses port 8080
 
 Connect using netcat (nc):
   nc <server-ip-address> <port>
 
 Inside the chat:
   - Type a message and press Enter to send it.
   - Use `/name <your_name>` to change your name.
   - Press Ctrl+C to disconnect.
 
 Make sure the selected port is allowed through the server's firewall.
 */

int main(int argc, char** argv)
{
    try
    {
        LOGLN("Starting chat server...");

        const int port = std::stoi(argc > 1 ? argv[1] : "9090");

        if (port < 1 || port > 65535) {
            throw std::invalid_argument("invalid port");
        }

        {
            server server(static_cast<std::uint16_t>(port));
            server.run();
        }

        LOGLN("Server stopped");

        return 0;
    }
    catch (const std::exception& ex)
    {
        LOGLN("error: {}", ex.what());
        return 1;
    }
}