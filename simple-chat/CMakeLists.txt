add_executable(simple-chat 
    main.cpp 
)

target_link_libraries(simple-chat PRIVATE ${LIB_DEPS})