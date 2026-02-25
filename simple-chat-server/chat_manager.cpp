#include <iostream>
#include <thread>
#include <sstream>
#include "chat_manager.h"

std::string HelperSimpleChat::get_room_name(const char * c) {
	int16_t len_room;
	std::string room_name;

	memcpy(&len_room, &c[1], 2);
	room_name.assign(&c[3], len_room);

	return room_name;
}

std::string HelperSimpleChat::get_room_message(const char* c) {
	int len_message;
	std::string chat_message;

	memcpy(&len_message, &c[1], sizeof(int));
	chat_message.assign(&c[1 + sizeof(int)], len_message);

	return chat_message;
}

void PacketManager::init_packetid(PacketID id) {
	// Clearing buffer.
	memset(
		m_message_temp.data(),
		0x0,
		m_cursor_size
	);

	m_cursor_size = 1;
	m_message_temp[0] = (unsigned char)id;
}
void PacketManager::attach_message(const char* message) {
	int size_int;
	int16_t size_int16;

	int sizeof_t;
	if (m_size_of_size < sizeof(int)) {
		size_int16 = static_cast<int16_t>(strlen(message));
		sizeof_t = sizeof(int16_t);
		m_cursor_size += size_int16;
		/*m_message_temp.assign(
			//m_message_temp.begin() + 1,
			&size_int16,
			&size_int16+sizeof_t
		);*/
		memcpy(m_message_temp.data()+1, &size_int16, sizeof_t);
	} else {
		size_int = static_cast<int>(strlen(message));
		sizeof_t = sizeof(int);
		m_cursor_size += size_int;
		/*m_message_temp.insert(
			m_message_temp.begin() + 1,
			&size_int,
			&size_int + sizeof_t
		);*/
		memcpy(m_message_temp.data()+1, &size_int, sizeof_t);
	}

	m_cursor_size += sizeof_t;
	/*m_message_temp.insert(
		m_message_temp.begin()+1+sizeof_t,
		message,
		message+strlen(message)
	);*/
	memcpy(
		m_message_temp.data()+1+sizeof_t,
		message,
		strlen(message)+1
	);
}
void PacketManager::attach_message_size(bool int16, bool int_t) {
	if (int16) {
		m_size_of_size = sizeof(int16_t);
	} else if (int_t) {
		m_size_of_size = sizeof(int);
	} else {
		throw std::exception("Must inform at least one type!");
	}
}
const char* PacketManager::get_message_data() {
	return m_message_temp.data();
}
int PacketManager::get_size_cursor() {
	return m_cursor_size;
}

Peer::~Peer() {
	closesocket(m_peer_sock);
}

int Peer::get_my_socket() {
	return m_peer_sock;
}

void Peer::close_connection() {
	closesocket(m_peer_sock);
}

time_t Peer::last_activity() {
	return m_last_activity;
}

bool Peer::break_time() {
	time_t now = time(nullptr);
	time_t peer_last = this->last_activity();

	if ((now - peer_last) > 10000000000000000000) {//MAX_TIME_INACTIVITY) {
		return true;
	}

	return false;
}

void Peer::get_me_a_room(ChatRoom *cr) {
	m_in_room = cr;
	m_is_in_room = true;
}

std::string &Peer::get_my_nickname() {
	return m_nickname;
}

ChatRoom *Peer::get_my_room() {
	return m_in_room;
}

void Peer::server_send(const char* message, int message_size) {
	int bytes_sent = send(m_peer_sock, message, message_size, 0);
	if (bytes_sent < 0) {
		int errono = WSAGetLastError();
		std::wcerr << "[!] (Peer: " << gai_strerror(errono);
		//std::cerr << "[!] (Peer: " << m_nickname << ") sending invalid bytes or no bytes at all. Just ignore ?\n";
	}
}

bool Peer::am_i_in_a_room() {
	return m_is_in_room;
}

void Peer::quit_room() {
	if (m_is_in_room) {
		m_in_room->remove_peer_from_room(this);
		m_is_in_room = false;
	}
}

bool ChatRoom::add_peer_to_room(Peer* p) {
	m_peers.push_back(p);
	return true;
}

bool ChatRoom::remove_peer_from_room(Peer* p) {
	for (auto it = m_peers.begin(); it != m_peers.end(); ) {
		if ((*it) == p) {
			m_peers.erase(it);
			break;
		}
		++it;
	}

	return true;
}

void ChatRoom::stack_message(std::string &message, std::string &nickname) {
	m_pending_messages.push_back("{"+ nickname +"}: " + message + "\n");
}

void ChatRoom::room_dispatch_loop() {
	std::vector<char> message;
	if (m_peers.size() == 0) {
		return;
	}

	while (m_pending_messages.size() > 0) {
		std::string message_s(m_pending_messages.front());
		message.insert(message.end(), message_s.c_str(), message_s.c_str() + message_s.size());

		m_pending_messages.pop_front();

		if (m_pending_messages.size() == 0) {
			message.push_back(0x00); //null terminator.
		}
	}

	for (auto it = m_peers.begin(); it != m_peers.end() && message.size() > 0; ) {
		Peer *p = *it;

		m_pkt_mng.init_packetid(PacketID::pk_peer_message);
		m_pkt_mng.attach_message_size(false, true);
		m_pkt_mng.attach_message(message.data());
		p->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());

		++it;
	}
}

void ChatRoom::mock_messages() {
	std::string message("{mock}: Hello from thread.\n");
	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(1));

		m_pending_messages.push_back(message);
	}
}

void MainRoom::add_peer(std::unique_ptr<Peer> p) {
	// Making socket non-blocking;
	u_long mode = 1;
	ioctlsocket(p->get_my_socket(), FIONBIO, &mode);

	m_peers.push_back(std::move(p));
}

void MainRoom::close_and_kick(std::vector<std::unique_ptr<Peer>>::iterator &it) {
	Peer* p = it->get();
	p->close_connection();

	if (p->am_i_in_a_room()) {
		// Since is just one room. Chat limitations.
		p->get_my_room()->remove_peer_from_room(p); //Could implement inside peer, but i don't feel like it
	}

	it = m_peers.erase(it);
}

bool MainRoom::create_chat_room(std::string &room_name) {
	std::unique_ptr<ChatRoom> c = std::make_unique<ChatRoom>(room_name);
	auto pair_check = m_chatrooms.emplace(std::string(room_name), std::move(c));

	//TODO: remove
	//std::thread t(&ChatRoom::mock_messages, pair_check.first->second.get());
	//t.detach();

	return pair_check.second;
}

bool MainRoom::join_chat_room(std::string& room_name, Peer *p) {
	ChatRoom *chat_room = m_chatrooms.find(room_name)->second.get();
	if (!chat_room->add_peer_to_room(p)) {
		return false;
	}
	p->get_me_a_room(chat_room);

	return true;
}

void MainRoom::parse_peer_messages(const char* buffer_packet, Peer *peer, std::vector<std::unique_ptr<Peer>>::iterator &itr) {
	PacketID pkt;
	
	// Get packet
	memcpy(&pkt, buffer_packet, 1);
	
	switch (pkt) {
	case PacketID::pk_create_room: {
		std::string room_name(HelperSimpleChat::get_room_name(buffer_packet));

		if (m_chatrooms.find(room_name) != m_chatrooms.end()) {
			m_pkt_mng.init_packetid(PacketID::pk_peer_message);
			m_pkt_mng.attach_message_size(false, true);
			m_pkt_mng.attach_message("[!] This chat room already exists!\n");
			peer->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());
		}

		if (!this->create_chat_room(room_name)) {
			m_pkt_mng.init_packetid(PacketID::pk_peer_message);
			m_pkt_mng.attach_message_size(false, true);
			m_pkt_mng.attach_message("[!] Chat room couldn't be created!\n");
			peer->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());
		} else {
			m_pkt_mng.init_packetid(PacketID::pk_peer_message);
			m_pkt_mng.attach_message_size(false, true);
			m_pkt_mng.attach_message("[*] Chat room created join in!\n");
			peer->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());
		}
		break;
	}
	case PacketID::pk_join_room: {
		std::string room_name(HelperSimpleChat::get_room_name(buffer_packet));
		
		if (m_chatrooms.find(room_name) == m_chatrooms.end()) {
			m_pkt_mng.init_packetid(PacketID::pk_peer_message);
			m_pkt_mng.attach_message_size(false, true);
			m_pkt_mng.attach_message("[!] This chat room doesn't exists!\n");
			peer->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());
		}

		if (!this->join_chat_room(room_name, peer)) {
			m_pkt_mng.init_packetid(PacketID::pk_peer_message);
			m_pkt_mng.attach_message_size(false, true);
			m_pkt_mng.attach_message("[!] Can't join room! Bye!\n");
			peer->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());
		} else {
			m_pkt_mng.init_packetid(PacketID::pk_join_room);
			m_pkt_mng.attach_message_size(false, true);
			m_pkt_mng.attach_message(room_name.c_str());
			peer->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());
			
			m_pkt_mng.init_packetid(PacketID::pk_peer_message);
			m_pkt_mng.attach_message_size(false, true);
			m_pkt_mng.attach_message("[*] Chat room joined! CHAT! ;]\n");
			peer->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());
		}

		break;
	}
	case PacketID::pk_exit_room: {
		peer->quit_room();
		
		m_pkt_mng.init_packetid(PacketID::pk_exit_room);
		peer->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());

		m_pkt_mng.init_packetid(PacketID::pk_peer_message);
		m_pkt_mng.attach_message_size(false, true);
		m_pkt_mng.attach_message("[*] You exited the room you were in.\n");
		peer->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());
		break;
	}
	case PacketID::pk_server_message: {
		if (!peer->am_i_in_a_room()) {
			return;
		}

		ChatRoom* cr = peer->get_my_room();
		if (cr == nullptr) {
			std::cerr << "[!] Trying to message a room that peer is not in. Debug it!" << std::endl;
			
			m_pkt_mng.init_packetid(PacketID::pk_peer_message);
			m_pkt_mng.attach_message_size(false, true);
			m_pkt_mng.attach_message("[!] You are trying to message a room that you are not in. Bye!\n");
			peer->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());
			this->close_and_kick(itr);
		}
		
		std::string message_to_room(HelperSimpleChat::get_room_message(buffer_packet));
		cr->stack_message(message_to_room, peer->get_my_nickname());

		break;
	}
	case PacketID::pk_get_list_commands: {
		m_pkt_mng.init_packetid(PacketID::pk_peer_message);
		m_pkt_mng.attach_message_size(false, true);
		m_pkt_mng.attach_message(SimpleChatMessages::motd_instructions);
		peer->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());
		break;
	}
	case PacketID::pk_list_available_rooms: {
		std::stringstream chat_rooms("");
		for (auto it = m_chatrooms.begin(); it != m_chatrooms.end(); ) {
			chat_rooms << "| " << (*it).first << "\n";
			++it;
		}

		m_pkt_mng.init_packetid(PacketID::pk_peer_message);
		m_pkt_mng.attach_message_size(false, true);
		m_pkt_mng.attach_message(chat_rooms.str().c_str());
		peer->server_send(m_pkt_mng.get_message_data(), m_pkt_mng.get_size_cursor());
		break;
	}
	}
}

void MainRoom::chat_loop() {
	std::vector<char> buffer_peer_pkmsg(MAX_LEN_BUFFER);
	int bytes_read;

	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		
		//=====================================
		// Looping through peers to translate
		// packets and all that stuff.
		//=====================================
		//std::vector<std::unique_ptr<Peer>>::const_iterator
		for (auto it = m_peers.begin(); it != m_peers.end(); ) {
			Peer* p = it->get();

			bytes_read = recv(p->get_my_socket(), buffer_peer_pkmsg.data(), MAX_LEN_BUFFER, 0);
			if (bytes_read < 0 && WSAGetLastError() == WSAEWOULDBLOCK) {
				if (p->break_time()) {
					this->close_and_kick(it);
				}
				++it;
				continue;
			} else if (bytes_read < 0) {
				this->close_and_kick(it);
				continue;
			} else if (bytes_read == 0) { //closed socket
				this->close_and_kick(it);
				continue;
			}
			this->parse_peer_messages(buffer_peer_pkmsg.data(), p, it);
			++it;
		}
		// End looping translator -------------------

		//=======================================
		// Loop to dispatch all rooms messages
		//=======================================
		for (auto it = m_chatrooms.begin(); it != m_chatrooms.end(); ) {
			ChatRoom *cr = it->second.get();
			cr->room_dispatch_loop();
			++it;
		}
		//End dispatch messages------------------

	}
}