#pragma once

#define MAX_LEN_BUFFER 32000
#define MAX_COMMAND_LEN_BUFFER 500

enum class PacketID : unsigned char {
	pk_ack_login = 0x1,
	pk_peer_message = 0x2,

	pk_get_list_commands = 0x3,
	pk_exit_room = 0x4,
	pk_list_available_rooms = 0x5,
	pk_create_room = 0x6,
	pk_join_room = 0x7,

	pk_server_message = 0x8,

	pk_check_alive = 0xFF
};