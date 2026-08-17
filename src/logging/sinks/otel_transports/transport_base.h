#ifndef OTEL_TRANSPORT_BASE_H
#define OTEL_TRANSPORT_BASE_H

//#define OTEL_DEBUG_TRANSPORT

#include <format>
#include <string>
#include <string_view>

#include "logging/sinks/sinks_opts.h"

namespace xtd {

struct otel_transport_base {
    std::string m_host;
    std::string m_path;
    std::string m_request_prefix;
    bool m_ipv6 = false;
    int m_port = 80;

    virtual ~otel_transport_base() = default;
    virtual void send(std::string_view json_body) const = 0;

    static std::string host_header_for(std::string_view host, bool is_ipv6, int port)
    {
        return is_ipv6
            ? std::format("[{}]:{}", host, port)
            : std::format("{}:{}", host, port);
    }

    void build_request_prefix(const otel_sink_opts& opts)
    {
        const std::string host_header = host_header_for(m_host, m_ipv6, m_port);

        m_request_prefix = std::format(
            "POST {} HTTP/1.1\r\n"
            "Host: {}\r\n"
            "Authorization: {}\r\n"
            "stream-name: {}\r\n"
            "Content-Type: application/json\r\n"
            "Connection: close\r\n",
            m_path, host_header,
            opts.auth_token, opts.service_namespace);
    }
};

} // namespace xtd

#endif
