#ifndef TCPCONNECTION_HPP
# define TCPCONNECTION_HPP

# include "webserv.hpp"

//represent one connection of one client, using TCP
class	TCPConnection {

private:

	int				_tcp_socket; // one connection of one client, using TCP
	ServerConfig	_config;// WARNING : A chaque requete il se peut que le client parle a un nouveau serveur ?
	//non car une TCPconnection cree par client connecte sur un port

	time_t			_end_of_request_time; // at what time has the last request arrived
	time_t			_header_start_time;//at what time the connection arrives
	time_t			_body_start_time; // at what time the body arrives
	time_t			_last_tcp_chunk_time;//at what time the last chunk arrives
	
	char			_buff[BUFF_SIZE];
	int				_bytes_received;
	int				_status; //statut de la livraison de la donnee
	
	Request			_request;	// current request, initialized by initialize_transfer()

	Response		_response; // current response, must be initialized by another function


	int				_body_protocol; // is content_length precised, or is it a chunked body

public:
	TCPConnection(int tcp_socket, ServerConfig config);
	~TCPConnection();

	void 	initialize_transfer();//One request per TCPConnection
	
	void	check_body_headers();//if POST : check the headers and update the encoding process of the body
	void 	use_recv();
	void	read_header();
	void	read_body();
	void	end_transfer();

	void	set_error(int error_code);
	bool 	is_valid_length(const std::string& content_length);

	Request	getRequest() const;
	int		get_status() const;
	int		getTCPSocket() const;
	time_t	getEndOfRequestTime() const;  
	time_t	getHeaderTime() const;
	time_t	getBodyTime() const;
	time_t	getLastChunkTime() const;

	int		getBodyProtocol() const;
	void	setBodyProtocol(int);


};

#endif
