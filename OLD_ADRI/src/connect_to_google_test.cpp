
#include "webserv.hpp"

int	main() {

	// CLIENT
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo("google.com", "80", &hints, &res)) {
		std::cerr << "error connecting to google" << std::endl;
		return 1;
	}
	
	struct addrinfo *rp;
	rp = res;
	int count = 1;
	while (rp) {
		 char ipstr[INET6_ADDRSTRLEN];
			void	*addr;
			const char *ipver;
		if (rp->ai_family == AF_INET) {  // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)rp->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else {  // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)rp->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
        
        // Convert the IP to a string
        inet_ntop(rp->ai_family, addr, ipstr, sizeof(ipstr));
        std::cout << count++ << ". " << ipver << ": " << ipstr << "\n";
		rp = rp->ai_next;
	}
	// -> returns 2 IP adresses because google.com supports 2 ip adresses versions
	return 0;
}

// The bind adress is not what the client uses to connect !
