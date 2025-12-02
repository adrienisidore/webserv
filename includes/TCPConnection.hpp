#ifndef TCPCONNECTION_HPP
# define TCPCONNECTION_HPP

# include "webserv.hpp"
# include "./Request.hpp"
# include "./Response.hpp"
//represent one connection of one client, using TCP
class	TCPConnection {

private:

	int				_tcp_socket; // one connection of one client, using TCP
	//non car une TCPconnection cree par client connecte sur un port

	time_t			_end_of_request_time; // at what time has the last request arrived
	time_t			_header_start_time;//at what time the connection arrives
	time_t			_body_start_time; // at what time the body arrives
	time_t			_cgi_start_time;
	time_t			_last_tcp_chunk_time;//at what time the last chunk arrives
	
	time_t			_header_max_time;
	time_t			_body_max_time;
	time_t			_between_chunks_max_time;
	time_t			_no_request_max_time;

	time_t			_cgi_max_time;
	
	char			_buff[BUFF_SIZE];
	int				_bytes_received;
	int				_status; //statut de la livraison de la donnee
	
	int				_body_protocol; // is content_length precised, or is it a chunked body
	
public:

	Response		_response; // current response, must be initialized by another function
	Request			_request;	// current request, initialized by initialize_transfer()
	std::map<int, CGI>	_map_cgi_fds_to_add; //all sockets we monitor, wrapped up for poll()
	ServerConfig	_config;// WARNING : A chaque requete il se peut que le client parle a un nouveau serveur ?

	struct sockaddr_storage _client_addr;
	socklen_t               _client_addr_len;


	// TCPConnection(int tcp_socket, ServerConfig config);
	TCPConnection(int fd, const ServerConfig &config, const struct sockaddr_storage &addr, socklen_t addr_len);
	~TCPConnection();

	void 	initialize_transfer();//One request per TCPConnection
	time_t	ret_time_directive(std::string directive_name, int defaul) const;
	void	execute_method();
	
	void	check_body_headers();//if POST : check the headers and update the encoding process of the body
	void 	use_recv();
	void	read_header();
	void	read_body(bool state);
	void	end_transfer();

	void	set_error(int error_code);
	bool 	is_valid_length(const std::string& content_length);

	Request	getRequest() const;
	Response getResponse() const;
	int		get_status() const;
	void	setStatus(int);
	int		getTCPSocket() const;

	time_t	getEndOfRequestTime() const;  
	time_t	getCGITime() const ;
	time_t	getHeaderTime() const;
	time_t	getBodyTime() const;
	time_t	getLastChunkTime() const;

	time_t	getHeaderMaxTime() const;
	time_t	getBodyMaxTime() const;
	time_t	getBetweenChunksMaxTime() const;
	time_t	getNoRequestMaxTime() const;
	time_t	getCGIMaxTime() const;


	int		getBodyProtocol() const;
	void	setBodyProtocol(int);

};

#endif
