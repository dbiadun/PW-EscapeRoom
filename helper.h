//
// Created by dawid on 14.01.19.
//

#ifndef ESCAPEROOM_HELPER_H
#define ESCAPEROOM_HELPER_H

#include <stddef.h>
#include <semaphore.h>

#define PLAYER_NUMBER_MAX_SIZE 4

/************************ MEMORY ***************************/

void deleteMemory(const char *name);

void *createMemory(const char *name, size_t size);

void *getMemory(const char *name, size_t size);

void closeMemory(void *ptr, size_t size);

/**********************************************************/


/*********************** SEMAPHORES ************************/

#define MUTEX_SEM "/pw_escape_room_sem_mutex"
#define FOR_EXITING_PLAYERS_SEM "/pw_escape_room_sem_for_exiting_players"
#define FOR_PROPOSITIONS_ADDING_SEM "/pw_escape_room_sem_for_propositions_adding"
#define AFTER_PROPOSITIONS_ADDING_SEM "/pw_escape_room_sem_after_propositions_adding"
#define FOR_GAME_SEM "/pw_escape_room_sem_for_game"
#define FOR_PLAYERS_IN_ROOM_SEM "/pw_escape_room_sem_for_players_in_room"

typedef sem_t *semaphore;

void P(semaphore sem);

void V(semaphore sem);

semaphore createSemaphore(const char *name, unsigned int value);

semaphore *createSemaphoresArray(const char *name, unsigned int value, unsigned int size);

void deleteSemaphore(const char *name);

void deleteSemaphoresArray(const char *name, unsigned int size);

semaphore getSemaphore(const char *name);

semaphore *getSemaphoresArray(const char *name, unsigned int size);

void closeSemaphore(semaphore s);

void closeSemaphoresArray(semaphore *s, unsigned int size);

/*************************************************************/


/*********************** ROOMS *************************/

#define FREE_ROOMS "/pw_escape_room_free_rooms"

typedef struct Room {
    int number;
    int prev;
    int next;
    int type;
    int capacity;
    int neededPlayers;
    int playersWaitingInside;
    int playersPlayingInside;
    int initiator;
} Room;

Room **createRoomsList(const char *name, int size);

void deleteRoomsList(const char *name, int size);

void closeRoomsList(Room **rooms, int size);

void addRoom(Room **rooms, int roomNumber);

void removeRoom(Room **rooms, int roomNumber);

int getRoomNumberWithSizeAndType(Room **rooms, int minSize, int type);

Room *getRoomFromNumber(Room **rooms, int roomNumber);

int roomEmpty(const Room *room);

void freeRoom(Room **rooms, int roomNumber, int *pendingGames);

/************************************************************/


/************************* PLAYERS *****************************/

#define FREE_PLAYERS "/pw_escape_room_free_players"

typedef struct Player {
    int number;
    int prev;
    int next;
    int type;
    int busy;
    int canPropose;
    int propositions;
    int currentRoom;
} Player;

Player **createPlayersList(const char *name, int size);

void deletePlayersList(const char *name, int size);

void closePlayersList(Player **players, int size);

void addPlayer(Player **players, int playersCount, int playerNumber);

void removePlayer(Player **players, int playerNumber);

/***************************************************************/


/*********************** PROPOSITIONS **************************/

#define OPEN_PROPOSITIONS "/pw_escape_room_open_propositions"

typedef struct Proposition {
    int initiator; // also index in the array
    int prev;
    int next;
    int roomType;
    int playersCount;
    int *players; // negative values for types
    int active;
} Proposition;

Proposition **createPropositionsList(const char *name, int size);

void deletePropositionsList(const char *name, int size);

void closePropositionsList(Proposition **propositions, int size);

void addProposition(Proposition **propositions, Player **playersList, int allPlayersCount, int playerNumber,
        int roomType, int playersCount, const int *players);

void removeProposition(Proposition **propositions, int playerNumber);

int findAndExecuteProposition(Proposition **propositions, Room **freeRooms, Player **players, int *freePlayersOfType,
        int *pendingGames);

/***************************************************************/


#endif //ESCAPEROOM_HELPER_H
