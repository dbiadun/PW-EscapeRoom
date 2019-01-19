//
// Created by dawid on 14.01.19.
//

#ifndef ESCAPEROOM_HELPER_H
#define ESCAPEROOM_HELPER_H

#include <stddef.h>
#include <semaphore.h>

#define PLAYER_NUMBER_MAX_SIZE 4

/************************ MEMORY ***************************/

#define SHARED_VARIABLES "/pw_escape_room_shared_variables"

typedef struct Shared {
    int playersCount;
    int roomsCount;
    int typesCount;
    int waitingForPropositionsVerification;
    int waitingForPropositionsAdding;
    int waitingAfterPropositionsAdding;
    int playersWithPropositions;
    int pendingGames;
    int exitingPlayer;
    int gamesOfExitingPlayer;
} Shared;

void deleteMemory(const char *name);

void *createMemory(const char *name, size_t size);

void *getMemory(const char *name, size_t size);

void closeMemory(void *ptr, size_t size);

/**********************************************************/


/*********************** SEMAPHORES ************************/

#define MUTEX_SEM "/pw_escape_room_sem_mutex"
#define FOR_PROPOSITIONS_VERIFICATION_SEM "/pw_escape_room_sem_for_propositions_verification"
#define FOR_EXITING_PLAYERS_SEM "/pw_escape_room_sem_for_exiting_players"
#define FOR_PROPOSITIONS_ADDING_SEM "/pw_escape_room_sem_for_propositions_adding"
#define AFTER_PROPOSITIONS_ADDING_SEM "/pw_escape_room_sem_after_propositions_adding"
#define FOR_GAME_SEM "/pw_escape_room_sem_for_game"
#define FOR_PLAYERS_IN_ROOM_SEM "/pw_escape_room_sem_for_players_in_room"

typedef sem_t *semaphore;

void P(semaphore sem);

void V(semaphore sem);

semaphore createSemaphore(const char *name, unsigned int value);

semaphore *createSemaphoresArray(const char *name, unsigned int value, int size);

void deleteSemaphore(const char *name);

void deleteSemaphoresArray(const char *name, int size);

semaphore getSemaphore(const char *name);

semaphore *getSemaphoresArray(const char *name, int size);

void closeSemaphore(semaphore s);

void closeSemaphoresArray(semaphore *s, int size);

/*************************************************************/


/**************************** TYPES ****************************/

#define TYPES "/pw_escape_room_types"

typedef struct Type {
    int number;
    int propositions;
    int freePlayers;
} Type;

Type *createTypesArray(const char *name, int size);

Type *getTypesArray(const char *name, int size);

void deleteTypesArray(const char *name);

void closeTypesArray(Type *types, int size);

int typeNumberFromChar(int character);

int typeNumberFromPlayerNumber(int playerNumber);

int typeNumberToPlayerNumber(int typeNumber);

/***************************************************************/


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
    int waitingInside;
} Player;

Player **createPlayersList(const char *name, int size);

Player **getPlayersList(const char *name, int size);

void deletePlayersList(const char *name, int size);

void closePlayersList(Player **players, int size);

void addPlayer(Player **players, int playersCount, int playerNumber);

void removePlayer(Player **players, int playerNumber);

/***************************************************************/


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

Room **getRoomsList(const char *name, int size);

void deleteRoomsList(const char *name, int size);

void closeRoomsList(Room **rooms, int size);

void addRoom(Room **rooms, int roomNumber);

void removeRoom(Room **rooms, int roomNumber);

int getRoomNumberWithSizeAndType(Room **rooms, int minSize, int type);

Room *getRoomFromNumber(Room **rooms, int roomNumber);

int roomEmpty(const Room *room);

void freeRoom(Room **rooms, Player **players, int roomNumber, int *pendingGames, semaphore *forGame);

/************************************************************/


/*********************** PROPOSITIONS **************************/

#define OPEN_PROPOSITIONS "/pw_escape_room_open_propositions"

typedef struct Proposition {
    int initiator; // also index in the array
    int prev;
    int next;
    int roomType;
    int playersCount;
    int active;
} Proposition;

int **createPlayersForPropositions(const char *name, int playersCount);

Proposition **createPropositionsList(const char *name, int size);

int **getPlayersForPropositions(const char *name, int playersCount);

Proposition **getPropositionsList(const char *name, int size);

void deletePropositionsList(const char *name, int size);

void closePropositionsList(Proposition **propositions, int size, int **players);

void addProposition(Proposition **propositions, Player **playersList, Shared  *shared, int allPlayersCount,
        int playerNumber, int roomType, int playersCount, const int *players, Type *types,
        int **playersForPropositions, int *currentProposition, int propositionsCount);

void removeProposition(Proposition **propositions, Player **playersList, int playerNumber, Type *types,
        int **playersForPropositions);

int canExecute(Proposition *proposition, Room **freeRooms, Player **players, Type *types, int *playersForProposition);

int findAndExecuteProposition(Proposition **propositions, Room **freeRooms, Player **players, Type *types,
        int *pendingGames, int **playersForPropositions, semaphore *forGame);

/***************************************************************/


#endif //ESCAPEROOM_HELPER_H
