#include "chat/server.h"

int main(int argc, char** argv)
{
    try
    {
        const int port = std::stoi(argc > 1 ? argv[1] : "9090");

        if (port < 1 || port > 65535) {
            throw std::invalid_argument("invalid port");
        }

        {
            server server(static_cast<std::uint16_t>(port));
            server.run();
        }
        println_locked("Server stopped");
        

        return 0;
    }
    catch (const std::exception& ex)
    {
        println_locked("error: {}", ex.what());
        return 1;
    }
}