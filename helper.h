//
// Created by dawid on 14.01.19.
//

#ifndef ESCAPEROOM_HELPER_H
#define ESCAPEROOM_HELPER_H

#include <stddef.h>

#define PLAYER_NUMBER_MAX_SIZE 4
#define SHM_NAME "/pw_escape_room_memory"



size_t memorySize();

void initializeMemory(int memoryFD, int playerCount, int roomsCount);

#endif //ESCAPEROOM_HELPER_H
