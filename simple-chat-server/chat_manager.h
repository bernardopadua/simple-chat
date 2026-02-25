#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <list>

#include "WinSock2.h"
#include "WS2tcpip.h"

#include "packets.h"
#include "static_messages.h"

#define MAX_TIME_INACTIVITY 120

class ChatRoom; // Foward declaration kkkcrying.

class HelperSimpleChat {
public:
	static std::string get_room_name(const char *c);
	static std::string get_room_message(const char *c);
};

class PacketManager {
public:
	PacketManager() :
		m_message_temp(MAX_LEN_BUFFER),
		m_size_of_size(0),
		m_cursor_size(0)
	{}

	void init_packetid(PacketID id);
	void attach_message(const char *message);
	void attach_message_size(bool int16=false, bool int_t=false);

	const char* get_message_data();
	int get_size_cursor();
private:
	std::vector<char> m_message_temp;
	int m_size_of_size;
	int m_cursor_size;
};

class Peer {
public:
	Peer(std::string p_nickname, struct sockaddr_storage p_peer_sockaddr, int p_peer_sock) :
		m_nickname(p_nickname),
		m_peer_sockaddr(p_peer_sockaddr),
		m_peer_sock(p_peer_sock),
		m_is_in_room(false),
		m_is_logged(false),
		m_last_activity(time(nullptr)),
		m_in_room(nullptr)

	{};
	~Peer();

	int get_my_socket();
	void close_connection();

	void server_send(const char* message, int message_size);
	bool break_time();
	void get_me_a_room(ChatRoom *cr);
	bool am_i_in_a_room();
	void quit_room();

	time_t last_activity();
	ChatRoom* get_my_room();
	std::string &get_my_nickname();

private:
	std::string m_nickname;
	bool m_is_in_room;
	bool m_is_logged;
	time_t m_last_activity;

	struct sockaddr_storage m_peer_sockaddr;
	int m_peer_sock;

	ChatRoom *m_in_room;
};

class ChatRoom {
public:
	ChatRoom(std::string room_name) :
		m_room_name(room_name)
	{}

	bool add_peer_to_room(Peer *p);
	bool remove_peer_from_room(Peer *p);
	void stack_message(std::string& message, std::string& nickname);

	void room_dispatch_loop();

	void mock_messages();
private:
	std::string m_room_name;
	PacketManager m_pkt_mng;

	std::vector<Peer*> m_peers;
	std::list<std::string> m_pending_messages;
};

class MainRoom {
public:
	void chat_loop();
	void add_peer(std::unique_ptr<Peer> p);
	void parse_peer_messages(const char* buffer_packet, Peer* p, std::vector<std::unique_ptr<Peer>>::iterator &itr);

	void close_and_kick(std::vector<std::unique_ptr<Peer>>::iterator &it);

	bool create_chat_room(std::string &room_name);
	bool join_chat_room(std::string &room_name, Peer *p);
private:
	PacketManager m_pkt_mng;
	std::vector<std::unique_ptr<Peer>> m_peers;
	std::unordered_map<std::string, std::unique_ptr<ChatRoom>> m_chatrooms;
};