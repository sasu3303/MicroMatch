#include "micromatch/engine.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <charconv>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
namespace {
class Socket {
    int fd_;
public:
    explicit Socket(int fd) : fd_(fd) { if (fd < 0) throw std::runtime_error("socket operation failed"); }
    Socket(Socket const&) = delete;
    Socket& operator=(Socket const&) = delete;
    Socket(Socket&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    ~Socket() { if (fd_ >= 0) ::close(fd_); }
    int get() const noexcept { return fd_; }
};
bool send_all(int fd, std::string const& text) {
    std::size_t sent = 0;
    while (sent < text.size()) {
        auto n = ::send(fd, text.data() + sent, text.size() - sent, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}
void connection(int fd, micromatch::Engine& engine) {
    timeval timeout{60, 0};
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    std::string pending; char buffer[1024];
    for (;;) {
        auto bytes = ::recv(fd, buffer, sizeof(buffer), 0);
        if (bytes < 0 && errno == EINTR) continue;
        if (bytes <= 0) return;
        for (decltype(bytes) i = 0; i < bytes; ++i) {
            char c = buffer[i];
            if (c != '\n') {
                pending += c;
                if (pending.size() > 256) { send_all(fd, "{\"ok\":false,\"code\":\"line_too_long\"}\n"); return; }
                continue;
            }
            if (!pending.empty() && pending.back() == '\r') pending.pop_back();
            if (pending == "QUIT") { send_all(fd, "{\"ok\":true,\"code\":\"bye\"}\n"); return; }
            micromatch::Response result;
            try { result = engine.submit(micromatch::parse(pending)).get(); }
            catch (std::invalid_argument const& e) { result.ok = false; result.code = e.what(); }
            if (!send_all(fd, micromatch::json(result) + '\n')) return;
            pending.clear();
        }
    }
}
}
int main(int argc, char** argv) {
    try {
        if (argc > 3) throw std::runtime_error("Usage: micromatch_server [port] [--once]");
        int port = 9000;
        if (argc >= 2) {
            std::string value = argv[1];
            auto [end, ec] = std::from_chars(value.data(), value.data()+value.size(), port);
            if (ec != std::errc{} || end != value.data()+value.size() || port < 0 || port > 65535) throw std::runtime_error("Invalid port");
        }
        bool once = argc == 3 && std::string(argv[2]) == "--once";
        if (argc == 3 && !once) throw std::runtime_error("Unknown option");
        std::signal(SIGPIPE, SIG_IGN);
        Socket listener(::socket(AF_INET, SOCK_STREAM, 0));
        int reuse = 1; ::setsockopt(listener.get(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        sockaddr_in address{}; address.sin_family = AF_INET; address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); address.sin_port = htons(static_cast<std::uint16_t>(port));
        if (::bind(listener.get(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) throw std::runtime_error("Bind failed; is the port already in use?");
        if (::listen(listener.get(), 8) < 0) throw std::runtime_error("Listen failed");
        socklen_t length = sizeof(address);
        ::getsockname(listener.get(), reinterpret_cast<sockaddr*>(&address), &length);
        std::cout << "LISTENING 127.0.0.1 " << ntohs(address.sin_port) << std::endl;
        micromatch::Engine engine;
        do {
            int accepted;
            do { accepted = ::accept(listener.get(), nullptr, nullptr); } while (accepted < 0 && errno == EINTR);
            Socket client(accepted);
            connection(client.get(), engine);
        } while (!once);
    } catch (std::exception const& e) { std::cerr << "Fatal: " << e.what() << '\n'; return 1; }
}
