add_executable(simple-chat-server 
    main.cpp
    chat_manager.cpp
    chat_manager.h
    static_messages.h
)

target_link_libraries(simple-chat-server PRIVATE ${LIB_DEPS})