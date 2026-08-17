#ifndef OTEL_TRANSPORT_BASE_H
#define OTEL_TRANSPORT_BASE_H

#include <string>
#include <string_view>

namespace xtd {

struct otel_transport_base {
    std::string m_host;
    std::string m_path;
    std::string m_request_prefix;
    bool m_ipv6 = false;
    int m_port = 80;

    virtual ~otel_transport_base() = default;
    virtual void send(std::string_view json_body) const = 0;
};

} // namespace xtd

#endif
