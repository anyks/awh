// example.cpp
#include "http_parser.hpp"
#include <cstdio>
#include <cstring>

using namespace http_parser;

// Callback implementations
parse_result_t on_message_begin(context_t& ctx) {
    std::printf("--- New Message ---\n");
    return parse_result_t::OK;
}

parse_result_t on_url(context_t& ctx, std::string_view url) {
    std::printf("URL: %.*s\n", static_cast<int>(url.size()), url.data());
    return parse_result_t::OK;
}

parse_result_t on_header(context_t& ctx, std::string_view name, std::string_view value) {
    std::printf("Header: %.*s: %.*s\n", 
                static_cast<int>(name.size()), name.data(),
                static_cast<int>(value.size()), value.data());
    return parse_result_t::OK;
}

parse_result_t on_headers_complete(context_t& ctx) {
    std::printf("--- Headers Complete ---\n");
    std::printf("HTTP/%u.%u\n", ctx.http_major, ctx.http_minor);
    if (ctx.type == message_type_t::REQUEST) {
        std::printf("Method: %s\n", std::string(method_resolver::method_to_string(ctx.method)).c_str());
    } else {
        std::printf("Status: %u\n", ctx.status_code);
    }
    std::printf("Keep-Alive: %s\n", should_keep_alive(ctx) ? "true" : "false");
    return parse_result_t::OK;
}

parse_result_t on_body(context_t& ctx, std::string_view body) {
    std::printf("Body chunk (%zu bytes): %.*s\n", 
                body.size(), static_cast<int>(body.size()), body.data());
    return parse_result_t::OK;
}

parse_result_t on_message_complete(context_t& ctx) {
    std::printf("--- Message Complete ---\n\n");
    return parse_result_t::OK;
}

int main() {
    // Setup handler
    handler_t handler;
    handler.on_message_begin = on_message_begin;
    handler.on_url = on_url;
    handler.on_header = on_header;
    handler.on_headers_complete = on_headers_complete;
    handler.on_body = on_body;
    handler.on_message_complete = on_message_complete;
    
    // Initialize context
    context_t ctx;
    init_context(ctx);
    
    // Test request
    const char* request = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: TestClient/1.0\r\n"
        "Accept: text/html\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    
    parse_result_t result = parse(ctx, request, std::strlen(request), handler);
    
    if (result == parse_result_t::OK) {
        std::printf("Parsing completed successfully!\n");
    } else {
        std::printf("Parsing failed with error: %d\n", static_cast<int>(result));
    }
    
    return 0;
}
