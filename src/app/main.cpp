#include <iostream>
#include <thread>
#include <chrono>
#include "../carrierbridge/core.cpp"

#include <asio.hpp>
#include <iostream>

int main() {
    try {
        asio::io_context io;

        asio::ip::tcp::endpoint endpoint(
            asio::ip::tcp::v4(),
            8080 // safe test port
        );

        asio::ip::tcp::acceptor acceptor(io);

        // Open
        acceptor.open(endpoint.protocol());

        // Allow address reuse
        acceptor.set_option(asio::socket_base::reuse_address(true));

        // Bind
        acceptor.bind(endpoint);

        // Listen
        acceptor.listen();

        std::cout << "Server running on port 8080..." << std::endl;

        asio::ip::tcp::socket socket(io);
        acceptor.accept(socket);

        std::cout << "Client connected!" << std::endl;
    }
    catch (const std::system_error& e) {
        std::cerr << "SYSTEM ERROR: " << e.what()
            << " (code=" << e.code() << ")" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
    }

    return 0;
}

