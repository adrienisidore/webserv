#include "webserv.hpp"
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

// --- Helpers (C++98) ---

static std::string to_string(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

static std::string center_text(std::string text, int width) {
    if ((int)text.length() >= width)
        return text.substr(0, width);
    int padding = width - text.length();
    int padLeft = padding / 2;
    int padRight = padding - padLeft;
    return std::string(padLeft, ' ') + text + std::string(padRight, ' ');
}

static std::string truncate_start(std::string text, int width) {
    if ((int)text.length() <= width)
        return center_text(text, width);
    return text.substr(text.length() - width);
}

static std::string get_method_color(std::string method) {
    if (method == "GET") return "\033[1;32m";      // Green
    if (method == "POST") return "\033[1;34m";     // Blue
    if (method == "DELETE") return "\033[1;31m";   // Red
    return "\033[1;33m";                           // Yellow
}

struct BoxInfo {
    int timeout;
    std::string method;
    std::string ip;
    std::string port;
    std::string uri;
};


int visualize_timeout(TCPConnection *conn) {
    time_t now = std::time(NULL);
    int remaining;
    int next_timeout = -1;

    // Check all timers and find the smallest positive remaining time
    if (conn->getEndOfRequestTime()) {
        remaining = conn->getNoRequestMaxTime() - (now - conn->getEndOfRequestTime());
        if (next_timeout < 0 || (remaining >= 0 && remaining < next_timeout)) next_timeout = remaining;
    }
    if (conn->getHeaderTime()) {
        remaining = conn->getHeaderMaxTime() - (now - conn->getHeaderTime());
        if (next_timeout < 0 || (remaining >= 0 && remaining < next_timeout)) next_timeout = remaining;
    }
    if (conn->getBodyTime()) {
        remaining = conn->getBodyMaxTime() - (now - conn->getBodyTime());
        if (next_timeout < 0 || (remaining >= 0 && remaining < next_timeout)) next_timeout = remaining;
    }
    if (conn->getLastChunkTime()) {
        remaining = conn->getBetweenChunksMaxTime() - (now - conn->getLastChunkTime());
        if (next_timeout < 0 || (remaining >= 0 && remaining < next_timeout)) next_timeout = remaining;
    }
    if (conn->getCGITime()) {
        remaining = conn->getCGIMaxTime() - (now - conn->getCGITime());
        if (next_timeout < 0 || (remaining >= 0 && remaining < next_timeout)) next_timeout = remaining;
    }
    
    // If no timer is active (waiting for new request start), we can return a default or 0
    return next_timeout;
}

void ServerMonitor::visualize() {
    std::stringstream buffer;

    buffer << "\033[2J\033[1;1H";

	buffer << "map server configf size : " << _map_server_configs.size() << std::endl;
	buffer << "map connections    size : " << _map_connections.size() << std::endl;
	buffer << "map cgis           size : " << _map_cgis.size() << std::endl;

    std::vector<BoxInfo> boxes;

    for (std::map<int, TCPConnection *>::iterator it = _map_connections.begin(); it != _map_connections.end(); ++it) {
        TCPConnection *conn = it->second;
        Request req = conn->getRequest();
        std::string ip_port = req.getConfig().getDirective("listen");

        BoxInfo box;
        box.timeout = visualize_timeout(conn);
        box.method = req.getMethod();
        box.uri = req.getURI();

        size_t colon_pos = ip_port.find(':');
        if (colon_pos != std::string::npos) {
            box.ip = ip_port.substr(0, colon_pos);
            box.port = ip_port.substr(colon_pos + 1);
        } else {
            box.ip = "Unknown";
            box.port = "???";
        }
        
        if (box.ip == "localhost") box.ip = "127.0.0.1";

        boxes.push_back(box);
    }

    // Draw to Buffer
    const int box_inner_width = 11;
    const int boxes_per_row = 6;
    std::string reset_color = "\033[0m";

    if (boxes.empty()) {
        buffer << "\n   Waiting for connections...\n";
    } else {
        for (size_t i = 0; i < boxes.size(); i += boxes_per_row) {
            for (size_t j = i; j < i + boxes_per_row && j < boxes.size(); ++j) {
                buffer << "  " << center_text(to_string(boxes[j].timeout), box_inner_width) << "   ";
            }
            buffer << "\n";

            for (size_t j = i; j < i + boxes_per_row && j < boxes.size(); ++j) {
                buffer << " ┌───────────┐  ";
            }
            buffer << "\n";

            for (size_t j = i; j < i + boxes_per_row && j < boxes.size(); ++j) {
                std::string color = get_method_color(boxes[j].method);
                buffer << " │" << color << center_text(boxes[j].method, box_inner_width) << reset_color << "│  ";
            }
            buffer << "\n";

            for (size_t j = i; j < i + boxes_per_row && j < boxes.size(); ++j) {
                buffer << " │" << center_text(boxes[j].ip, box_inner_width) << "│  ";
            }
            buffer << "\n";

            for (size_t j = i; j < i + boxes_per_row && j < boxes.size(); ++j) {
                buffer << " │" << center_text(boxes[j].port, box_inner_width) << "│  ";
            }
            buffer << "\n";

            for (size_t j = i; j < i + boxes_per_row && j < boxes.size(); ++j) {
                buffer << " │" << truncate_start(boxes[j].uri, box_inner_width) << "│  ";
            }
            buffer << "\n";

            for (size_t j = i; j < i + boxes_per_row && j < boxes.size(); ++j) {
                buffer << " └───────────┘  ";
            }
            buffer << "\n\n";
        }
    }

    // print everything
    std::cout << buffer.str() << std::flush;
}
