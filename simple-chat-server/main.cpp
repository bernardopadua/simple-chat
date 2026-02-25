#include <iostream>
#include <thread>

#include "chat_manager.h" //WinSock n stuff
#include "packets.h"

//==================================
// Author: Monkey/Pimptech
// There is a lot to port to linux
// I know it! But I do it slow
//==================================

int main(int argv, char** argc) {
	
	WSAData wsaD;
	int status_init;

	int g_sock;
	int no; // setsockopt

	struct addrinfo hints, * res;
	
	status_init = WSAStartup(MAKEWORD(2, 2), &wsaD);

	if (status_init != 0) {
		std::cerr << "Error initilizing WSA.\n";
		return -1;
	}

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(NULL, "8899", &hints, &res);
	g_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	no = 0;
	setsockopt(g_sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&no, sizeof(no));

	if (g_sock < 0) {
		std::cerr << "Error trying to create a socket.\n";
		return -1;
	}

	status_init = bind(g_sock, res->ai_addr, res->ai_addrlen);

	if (status_init < 0) {
		std::cerr << "Error trying to bind socket.\n";
		return -1;
	}

	status_init = listen(g_sock, 10);

	if (status_init < 0) {
		std::cerr << "Error trying to listen.\n";
		return -1;
	}

	freeaddrinfo(res);

	//=====================================
	// Initializing MainRoom and chat loop
	//=====================================
	MainRoom g_mainroom;
	std::thread chat_loop(&MainRoom::chat_loop, &g_mainroom);
	chat_loop.detach();

	PacketManager pkt_mng; // Should I make it in MainRoom ? Don't know.. 

	while (1) {
		struct sockaddr_storage new_peer;
		int status_accept_sock; 
		int new_peer_len;
		int last_errno;

		new_peer_len = sizeof new_peer;
		std::cout << "[@] Waiting connections...\n";
		status_accept_sock = accept(g_sock, (sockaddr *)&new_peer, &new_peer_len);
		if (status_accept_sock < 0) {
			last_errno = WSAGetLastError();
			switch (last_errno) {
				case WSAENOTSOCK:
					std::cerr << "Socket is dead. Server must restart.\n";
					return -1;
				
				//... and so on.
			}

			std::cerr << "[!] Some mysterious error, with faith the server will keep up. Ignore for now.\n";
		}
		std::cout << "[@] 1 peer connected... \n";

		//==================================================
		// Error free so lets create peer.
		// Initializing peer.
		//==================================================
		char packet_init[MAX_COMMAND_LEN_BUFFER];
		int status_recv;
		int16_t len_nickname;
		std::string peer_nickname;
		std::unique_ptr<Peer> p;

		status_recv = recv(status_accept_sock, packet_init, MAX_COMMAND_LEN_BUFFER, 0);
		if (status_recv < 0 || status_recv == 0) {
			last_errno = WSAGetLastError();
			switch (last_errno) {
			case WSAENOTSOCK:
				std::cerr << "Socket is dead. Server must restart.\n";
				return -1;

				//... and so on.
			}

			std::cerr << "[!] Error recv, if error ocurred the client must try again. Bye.\n";
			break;
		}
		if ((unsigned char)packet_init[0] != (unsigned char)PacketID::pk_ack_login) {
			std::cerr << "[!] My friend is not emitting normal packets. Bye.";
			closesocket(status_accept_sock);
			break;
		}
		
		memcpy(&len_nickname, &packet_init[1], 2);
		if (len_nickname > 200) {
			std::cerr << "[!] Malformed packet or nickname len is too big. Investigate or debug it.\n";
			return -1;
		}
		
		peer_nickname.assign(&packet_init[3], len_nickname);
		p = std::make_unique<Peer>(peer_nickname, new_peer, status_accept_sock);
		
		pkt_mng.init_packetid(PacketID::pk_peer_message);
		pkt_mng.attach_message_size(false, true);
		pkt_mng.attach_message(SimpleChatMessages::motd_instructions);
		p->server_send(pkt_mng.get_message_data(), pkt_mng.get_size_cursor());
		// End Peer ---------------

		//================================
		// Initializing peer in MainRoom
		//================================
		g_mainroom.add_peer(std::move(p)); // I want to use move.
		// End MainRoom ------------------
	}
	// End MainRoom and chat loop ----------------------

	WSACleanup();

	return 0;
}