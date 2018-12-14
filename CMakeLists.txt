cmake_minimum_required(VERSION 3.13)
project(hw3 C)

set(CMAKE_C_STANDARD 99)

add_executable(hw3 main.c message_slot.c message_slot.h message_reader.c message_reader.h message_sender.c message_sender.h)