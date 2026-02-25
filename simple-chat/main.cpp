#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <list>

#include "WinSock2.h"
#include "WS2tcpip.h"
#include "conio.h"

#include "packets.h"

//==================================
// Author: Monkey/Pimptech
// There is a lot to port to linux
// I know it! But I do it slowly
//==================================

//===============================
// Classes.h
//===============================
class User {
public:
	User() :
		m_nickname(""),
		m_in_room("")
	{}

public:
	std::string m_nickname;
	std::string m_in_room;
};

class PacketSC { //SimpleChat
public:
	PacketSC(int size_message, PacketID id) :
		m_id_packet(id),
		m_packet_message(size_message)
	{};
	void assign_message(const char *message) {
		m_packet_message.assign(message, message+m_packet_message.size());
		if (m_packet_message.size() > 0 && m_packet_message[m_packet_message.size()-1] != 0x0) {
			m_packet_message.insert(m_packet_message.end(), 0x0);
		} else if (m_packet_message.size() == 0) {
			m_packet_message.insert(m_packet_message.end(), 0x0);
		}
	}
	const char* get_message() {
		return m_packet_message.data();
	}
private:
	PacketID m_id_packet;
	std::vector<char> m_packet_message;
};

// Globals
int g_sock;
User this_user;

int get_last_sock_error() {
	// To be ported to linux later.
	return WSAGetLastError();
}

bool is_type_key_pressed() {
	if (_kbhit()) {
		if (_getch() == 't') {
			return true;
		}
	}
	return false;
}

void flush_console_garbage() {
	// To be ported to linux later.
	FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
}

void assemble_and_send(const char *message, bool is_command=false) {
	int status_send;
	if (is_command) {
		status_send = send(g_sock, message, MAX_COMMAND_LEN_BUFFER, 0);
	} else {
		status_send = send(g_sock, message, MAX_LEN_BUFFER, 0);
	}

	if (status_send < 0) { //sock error
		int sock_error = get_last_sock_error();
		if (sock_error == WSAENOTSOCK) {
			std::cout << "[!] Bye." << std::endl;
			exit(-1);
		}
	}
}

std::string get_room_name(std::string &type_command) {
	std::string room_name;
	size_t pos = type_command.find(" ");

	if (pos != std::string::npos) {
		room_name.insert(0, &type_command[pos + 1]);
	}

	return room_name;
}

void parse_command(std::string typed_message) {
	if (typed_message[0] == '/') {
		char messagesend[MAX_COMMAND_LEN_BUFFER] = { 0 };
		char cmd[3];
		cmd[0] = typed_message[0];
		cmd[1] = typed_message[1];
		cmd[2] = 0x0;

		if (strcmp(cmd, "/q") == 0) {
			std::cout << "[!] Bye!" << std::endl;
			exit(-1);
		} else if (strcmp(cmd, "/x") == 0) {
			messagesend[0] = (unsigned char)PacketID::pk_exit_room;
			assemble_and_send(messagesend, true);
		} else if (strcmp(cmd, "/c") == 0) {
			std::string room_name = get_room_name(typed_message);
			int16_t len_room_name = static_cast<int16_t>(room_name.size());

			if (room_name.empty()) {
				std::cout << "[!] Your room is null or empty, please stop making fun of me. Bye!\n";
				return;
			}

			messagesend[0] = (unsigned char)PacketID::pk_create_room;
			memcpy(&messagesend[1], &len_room_name, 2);
			memcpy(&messagesend[3], room_name.c_str(), room_name.size());

			assemble_and_send(messagesend, true);
		} else if (strcmp(cmd, "/j") == 0) {
			std::string room_name = get_room_name(typed_message);
			int16_t len_room_name = static_cast<int16_t>(room_name.size());

			if (room_name.empty()) {
				std::cout << "[!] Your room is null or empty, please stop making fun of me.Bye!\n";
				return;
			}

			messagesend[0] = (unsigned char)PacketID::pk_join_room;
			memcpy(&messagesend[1], &len_room_name, 2);
			memcpy(&messagesend[3], room_name.c_str(), room_name.size());

			assemble_and_send(messagesend, true);
		} else if (strcmp(cmd, "/x") == 0) {
			messagesend[0] = (unsigned char)PacketID::pk_exit_room;
			assemble_and_send(messagesend, true);
		} else if (strcmp(cmd, "/l") == 0) {
			messagesend[0] = (unsigned char)PacketID::pk_get_list_commands;
			assemble_and_send(messagesend, true);
		} else if (strcmp(cmd, "/r") == 0) {
			messagesend[0] = (unsigned char)PacketID::pk_list_available_rooms;
			assemble_and_send(messagesend, true);
		}
	} else if (this_user.m_in_room != "") {
		char messagesend[MAX_LEN_BUFFER];
		int message_size = 0;

		messagesend[0] = (unsigned char)PacketID::pk_server_message;
		message_size = typed_message.size();
		memcpy(&messagesend[1], &message_size, sizeof(int)); // 1+4
		memcpy(&messagesend[5], typed_message.c_str(), message_size);
		assemble_and_send(messagesend, false);
	}
}

void parser_message(char* message, int bytes_read, std::list<PacketSC> &queue_packets) {
	//std::vector<char> &message_buffer) {
	PacketID pk_id;
	int first_packet_size = 0;
	memcpy(&pk_id, &message[0], 1);

	switch (pk_id)
	{
	case PacketID::pk_peer_message: {
		int message_size;
		
		//Message size
		memcpy(&message_size, &message[1], sizeof(int));

		// 5: 1 id + 4 (int) size
		first_packet_size = message_size + 5; // leave my magical number alone, pls hahah

		PacketSC np(message_size, pk_id);
		np.assign_message(&message[5]);
		queue_packets.push_back(np);
		/*message_buffer.assign(
			&message[5], // Id + sizeof 4 int
			&message[5] + message_size
		);*/
		break;
	}
	case PacketID::pk_get_list_commands:
		break;
	case PacketID::pk_exit_room: {
		this_user.m_in_room = "";
		first_packet_size = 1;
		break;
	}
	case PacketID::pk_join_room: {
		int message_size;

		//Message size
		memcpy(&message_size, &message[1], sizeof(int));

		first_packet_size = message_size + 5;

		this_user.m_in_room.assign(&message[5], message_size);

		break;
	}
	default:
		break;
	}

	// Next packet
	if ((char)message[first_packet_size] != 0x0) {
		//Anyways last call will clear everything, but (bytes_read-first_packet_size) avoids problems.
		parser_message(&message[first_packet_size], (bytes_read-first_packet_size), queue_packets);
	}

	//Clearing read buffer
	memset(message, 0x0, bytes_read);
}

int main(int argv, char **argc) {

	if (argv < 2) {
		std::cerr << "[!] Must inform your nickname. .exe nickname";
		return -1;
	}

	WSAData wsaData;

	int getaddr_st; //global socket
	struct addrinfo hints, * res;
	//std::string nickname(argc[1], strlen(argc[1]));
	this_user.m_nickname.assign(argc[1]);
	std::vector<char> buffer_messages(MAX_LEN_BUFFER);
	std::list<PacketSC> queue_packets;
	int buffer_messages_read;
	
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	hints.ai_family = AF_INET;
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_socktype = SOCK_STREAM;
	memset(&hints, 0, sizeof hints);
	
	getaddr_st = getaddrinfo("127.0.0.1", "8899", &hints, &res); // I could use IPV6, but I didn't hehe
	if (getaddr_st != 0) {
		std::cout << "error getaddrinfo:: ";
		std::wcout << gai_strerror(getaddr_st) << "\n";
	}

	g_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (g_sock < 0) {
		std::cerr << "Error trying to create a socket.\n";
		return -1;
	}
	
	int status_conn = connect(g_sock, res->ai_addr, res->ai_addrlen);
	if (status_conn < 0) {
		std::cerr << "[!] Some problem with connect. Bye, try again or debug it.\n";
		std::cerr << "Original error: ";
		std::wcerr << gai_strerror(WSAGetLastError());
		return -1;
	}

	freeaddrinfo(res);

	//===============================
	// ACK of login
	//===============================
	char bufacklogin[MAX_COMMAND_LEN_BUFFER];
	int16_t len_nickname = static_cast<int16_t>(this_user.m_nickname.size());//strlen(argc[1]);
	int bytes_sent_ack_login;

	bufacklogin[0] = (const unsigned char)PacketID::pk_ack_login;
	memcpy(&bufacklogin[1], (void*)&len_nickname, 2);
	memcpy(&bufacklogin[3], this_user.m_nickname.c_str(), len_nickname+1);

	bytes_sent_ack_login = send(g_sock, bufacklogin, MAX_COMMAND_LEN_BUFFER, 0);
	if (bytes_sent_ack_login < 0) {
		std::cerr << "[!] Error trying to send packet.\n";
		return -1;
	}
	// End ACK login ----------------

	int bytes_rec = recv(g_sock, buffer_messages.data(), MAX_COMMAND_LEN_BUFFER, 0);
	if (bytes_rec < 0) {
		std::cerr << "[!] Error trying to recv packets.\n";
		return -1;
	}

	parser_message(buffer_messages.data(), bytes_rec, queue_packets);
	PacketSC message_welcome = queue_packets.front();
	queue_packets.pop_front();
	std::cout << message_welcome.get_message();

	//===============================
	// Windows only way non-blocking
	//===============================
	u_long mode = 1;
	ioctlsocket(g_sock, FIONBIO, &mode);
	
	std::string user_typed;

	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		if (is_type_key_pressed()) {
			flush_console_garbage();
			
			if (this_user.m_in_room.size() > 0) {
				std::cout << "[room: " << this_user.m_in_room << "]> ";
			} else {
				std::cout << "@> ";
			}

			std::getline(std::cin, user_typed);

			// if I type T
			flush_console_garbage();

			parse_command(user_typed);
		}

		//========================
		// Reading messages
		//========================
		buffer_messages_read = recv(g_sock, buffer_messages.data(), MAX_LEN_BUFFER, 0);
		if (buffer_messages_read < 0) {
			int last_errno = WSAGetLastError();

			switch (last_errno) {
			case WSAEWOULDBLOCK: {
				//Nothing to do.
				continue;
			}
			case ECONNRESET: {
				std::cerr << "[!] Socket is dead. Bye.\n";
				exit(-1);
			}
			case WSAENOTSOCK: {
				std::cerr << "[!] Error recv, if error ocurred the client must try again. Bye.\n";
				exit(-1);
			}
			case WSAECONNRESET: {
				std::cout << "\n\n[!!] Connection with server closed. Bye!\n";
				exit(-1);
			}
			}
		} else {
			parser_message(buffer_messages.data(), buffer_messages_read, queue_packets);

			for (auto it = queue_packets.begin(); it != queue_packets.end(); ) {
				std::cout << (*it).get_message();
				it = queue_packets.erase(it);
			}
		}
	}

	WSACleanup();

	return 0;
}