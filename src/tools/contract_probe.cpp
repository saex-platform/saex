#include <saex/contracts/fixture_codec.hpp>
#include <algorithm>
#include <iostream>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

// One bounded fixture frame over stdin/stdout. No server socket or game injection.
int main() {
#ifdef _WIN32
    if (_setmode(_fileno(stdin), _O_BINARY) == -1 || _setmode(_fileno(stdout), _O_BINARY) == -1) return 5;
#endif
    using namespace saex::contracts;
    std::array<std::byte, fixture_frame_bytes> frame{};
    std::cin.read(reinterpret_cast<char*>(frame.data()), fixture_header_bytes);
    if (std::cin.gcount() != static_cast<std::streamsize>(fixture_header_bytes)) {
        std::cerr << "truncated_header\n"; return 2;
    }
    const auto header = validate_fixture_header(std::span{frame}.first(fixture_header_bytes));
    if (header != DecodeError::none) { std::cerr << "invalid_header\n"; return 3; }
    std::cin.read(reinterpret_cast<char*>(frame.data() + fixture_header_bytes), fixture_payload_bytes);
    if (std::cin.gcount() != static_cast<std::streamsize>(fixture_payload_bytes)) {
        std::cerr << "truncated_payload\n"; return 2;
    }
    if (std::cin.peek() != std::char_traits<char>::eof()) { std::cerr << "trailing_bytes\n"; return 3; }
    const auto decoded = decode_fixture(frame);
    if (decoded.error != DecodeError::none) { std::cerr << "invalid_entity\n"; return 4; }
    const auto output = encode_fixture(decoded.entity);
    std::cout.write(reinterpret_cast<const char*>(output.data()), output.size());
    return std::cout.good() ? 0 : 5;
}
