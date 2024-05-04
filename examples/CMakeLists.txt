cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0115 OLD)

add_executable(example1 example1)
target_link_libraries(example1 icon7)

add_executable(chat_server chat_server)
target_link_libraries(chat_server icon7)

add_executable(chat_client chat_client)
target_link_libraries(chat_client icon7)
