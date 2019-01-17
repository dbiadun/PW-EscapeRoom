//
// Created by dawid on 14.01.19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "helper.h"

#define MAX_NUMBER_SIZE 4


static char *catStringAndNumber(const char *string, int number) {
    char *newString = (char *) malloc((strlen(string) + 2 + MAX_NUMBER_SIZE) * sizeof(char));
    sprintf(newString, "%s_%d", string, number);
    return newString;
}

static char *catTwoStrings(const char *first, const char *second) {
    char *new = (char *) malloc((strlen(first) + strlen(second) + 2) * sizeof(char));
    sprintf(new, "%s_%s", first, second);
    return new;
}

static void deleteList(const char *name, int size) {
    for (int i = 0; i < size + 2; i++) {
        char *cellName = catStringAndNumber(name, i);
        deleteMemory(cellName);
        free(cellName);
    }
}


/************************* MEMORY ***************************/

void deleteMemory(const char *name) {
    int ret = shm_unlink(name);
    if (ret == -1) {
        perror("shm_open");
    }
}

void *createMemory(const char *name, size_t size) {
    int fd = shm_open(name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }

    int ret = ftruncate(fd, size);
    if (ret == -1) {
        perror("ftruncate");
        deleteMemory(name);
        exit(1);
    }

    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        deleteMemory(name);
        exit(1);
    }
    return ptr;
}

void *getMemory(const char *name, size_t size) {
    int fd = shm_open(name, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }

    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        deleteMemory(name);
        exit(1);
    }
    return ptr;
}

void closeMemory(void *ptr, size_t size) {
    int ret = munmap(ptr, size);
    if (ret == -1) {
        perror("munmap");
    }
}

/************************************************************/


/*********************** SEMAPHORES ************************/

void P(sem_t *sem) {
    if (sem_wait(sem) == -1) {
        perror("sem_wait in player");
        exit(1);
    }
}

void V(sem_t *sem) {
    if (sem_post(sem) == -1) {
        perror("sem_wait in player");
        exit(1);
    }
}

semaphore createSemaphore(const char *name, unsigned int value) {
    semaphore s = sem_open(name, O_CREAT | O_EXCL | O_RDWR, S_IWUSR | S_IRUSR, value);
    if (s == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }
    return s;
}

semaphore *createSemaphoresArray(const char *name, unsigned int value, unsigned int size) {
    semaphore *sArray = (semaphore *) malloc(size * sizeof(semaphore));
    for (int i = 0; i < size; i++) {
        char *cellName = catStringAndNumber(name, i);
        sArray[i] = createSemaphore(cellName, value);
        free(cellName);
    }
    return sArray;
}

void deleteSemaphore(const char *name) {
    int ret = sem_unlink(name);
    if (ret == -1) perror("sem_unlink");
}

void deleteSemaphoresArray(const char *name, unsigned int size) {
    for (int i = 0; i < size; i++) {
        char *cellName = catStringAndNumber(name, i);
        deleteSemaphore(cellName);
        free(cellName);
    }
}

semaphore getSemaphore(const char *name) {
    semaphore s = sem_open(name, O_RDWR);
    if (s == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }
    return s;
}

semaphore *getSemaphoresArray(const char *name, unsigned int size) {
    semaphore *sArray = (semaphore *) malloc(size * sizeof(semaphore));
    for (int i = 0; i < size; i++) {
        char *cellName = catStringAndNumber(name, i);
        sArray[i] = getSemaphore(cellName);
        free(cellName);
    }
    return sArray;
}

void closeSemaphore(semaphore s) {
    int ret = sem_close(s);
    if (ret == -1) perror("sem_close");
}

void closeSemaphoresArray(semaphore *s, unsigned int size) {
    for (int i = 0; i < size; i++) {
        closeSemaphore(s[i]);
    }
    free(s);
}

/************************************************************/


/*************************** ROOMS ****************************/

static int isLastRoom(Room *room) {
    return room->next == room->number;
}

static void takeRoom(Room **rooms, int roomNumber, int neededPlayers, int initiator, int *pendingGames) {
    removeRoom(rooms, roomNumber);
    Room *r = rooms[roomNumber];
    r->neededPlayers = neededPlayers;
    r->playersWaitingInside = 0;
    r->playersPlayingInside = 0;
    r->initiator = initiator;
    (*pendingGames)++;
}

Room **createRoomsList(const char *name, int size) {
    Room **list = (Room **) malloc((size + 2) * sizeof(Room *));
    for (int i = 0; i < size + 2; i++) {
        char *cellName = catStringAndNumber(name, i);
        list[i] = (Room *) createMemory(cellName, sizeof(Room));
        free(cellName);
        list[i]->number = i;
    }
    list[0]->prev = 0;
    list[0]->next = size + 1;
    list[size + 1]->prev = 0;
    list[size + 1]->next = size + 1;
    return list;
}

void deleteRoomsList(const char *name, int size) {
    deleteList(name, size);
}

void closeRoomsList(Room **rooms, int size) {
    for (int i = 0; i < size + 2; i++) {
        closeMemory(rooms[i], sizeof(Room));
    }
    free(rooms);
}

void addRoom(Room **rooms, int roomNumber) {
    int currentNumber = rooms[0]->next;
    while (!isLastRoom(rooms[currentNumber]) && rooms[roomNumber]->capacity <= rooms[currentNumber]->capacity)
        currentNumber = rooms[currentNumber]->next;
    int prev = rooms[currentNumber]->prev;
    int next = currentNumber;
    rooms[prev]->next = roomNumber;
    rooms[next]->prev = roomNumber;
    rooms[roomNumber]->prev = prev;
    rooms[roomNumber]->next = next;
}

void removeRoom(Room **rooms, int roomNumber) {
    int prev = rooms[roomNumber]->prev;
    int next = rooms[roomNumber]->next;
    rooms[prev]->next = next;
    rooms[next]->prev = prev;
}

int getRoomNumberWithSizeAndType(Room **rooms, int minSize, int type) {
    int currentNumber = rooms[0]->next;
    while (!isLastRoom(rooms[currentNumber]) &&
    (minSize > rooms[currentNumber]->capacity || rooms[currentNumber]->type != type))
        currentNumber = rooms[currentNumber]->next;
    if (isLastRoom(rooms[currentNumber])) return 0;
    return currentNumber;
}

Room *getRoomFromNumber(Room **rooms, int roomNumber) {
    return rooms[roomNumber];
}

int roomEmpty(const Room *room) {
    return room->playersPlayingInside == 0;
}

void freeRoom(Room **rooms, int roomNumber, int *pendingGames) {
    addRoom(rooms, roomNumber);
    (*pendingGames)--;
}

/***************************************************************/


/************************* PLAYERS *****************************/

static int isLastPlayer(Player *player) {
    return player->next == player->number;
}

static int getPlayerNumberWithType(Player **players, int type) {
    int currentNumber = players[0]->next;
    while (!isLastPlayer(players[currentNumber]) && players[currentNumber]->type != type)
        currentNumber = players[currentNumber]->next;
    if (isLastPlayer(players[currentNumber])) return 0;
    return currentNumber;
}

static void addPlayerToGame(Player *player, int roomNumber, int *freePlayersOfType) {
    player->busy = 1;
    player->currentRoom = roomNumber;
    freePlayersOfType[player->type]--;
}

Player **createPlayersList(const char *name, int size) {
    Player **list = (Player **) malloc((size + 2) * sizeof(Player *));
    for (int i = 0; i < size + 2; i++) {
        char *cellName = catStringAndNumber(name, i);
        list[i] = (Player *) createMemory(cellName, sizeof(Player));
        free(cellName);
        list[i]->number = i;
        list[i]->prev = i - 1;
        list[i]->next = i + 1;
        list[i]->busy = 0;
        list[i]->canPropose = 1;
        list[i]->propositions = 0;
    }
    list[0]->prev = 0;
    list[size + 1]->next = size + 1;
    return list;
}

void deletePlayersList(const char *name, int size) {
    deleteList(name, size);
}

void closePlayersList(Player **players, int size) {
    for (int i = 0; i < size + 2; i++) {
        closeMemory(players[i], sizeof(Player));
    }
    free(players);
}

void addPlayer(Player **players, int playersCount, int playerNumber) {
    int currentNumber = playersCount;
    int prev = players[currentNumber]->prev;
    int next = currentNumber;
    players[prev]->next = playerNumber;
    players[next]->prev = playerNumber;
    players[playerNumber]->prev = prev;
    players[playerNumber]->next = next;
}

void removePlayer(Player **players, int playerNumber) {
    int prev = players[playerNumber]->prev;
    int next = players[playerNumber]->next;
    players[prev]->next = next;
    players[next]->prev = prev;
}

/***************************************************************/


/*********************** PROPOSITIONS **************************/

#define PROPOSITIONS_PLAYERS_SUFIX "players"

static int isLastProposition(Proposition *p) {
    return p->next == p->initiator;
}

static int numberToPlayerType(int number) {
    return -number;
}

static int canExecute(Proposition *proposition, Room **freeRooms, Player **players, int *freePlayersOfType) {
    int playersCount = proposition->playersCount;
    int roomType = proposition->roomType;
    int roomNumber = getRoomNumberWithSizeAndType(freeRooms, playersCount, roomType);
    if (roomNumber == 0) return 0;
    int possible = 1;
    int *p = proposition->players;
    for (int i = 0; i < playersCount; i++) {
        if (p[i] > 0) {
            Player *player = players[p[i]];
            freePlayersOfType[player->type]--;
            if (player->busy || freePlayersOfType[player->type] < 0) possible = 0;
        } else if (p[i] < 0) {
            int t = (-1) * p[i];
            freePlayersOfType[t]--;
            if (freePlayersOfType[t] < 0) possible = 0;
        }
    }
    for (int i = 0; i < playersCount; i++) {
        if (p[i] > 0) {
            Player *player = players[p[i]];
            freePlayersOfType[player->type]++;
        } else if (p[i] < 0) {
            int t = (-1) * p[i];
            freePlayersOfType[t]++;
        }
    }
    return possible;
}

static void execute(Proposition *proposition, Room **freeRooms, Player **players, int *freePlayersOfType,
        int *pendingGames) {
    int initiator = proposition->initiator;
    int playersCount = proposition->playersCount;
    int roomType = proposition->roomType;
    int roomNumber = getRoomNumberWithSizeAndType(freeRooms, playersCount, roomType);
    takeRoom(freeRooms, roomNumber, playersCount, initiator, pendingGames);

    int *p = proposition->players;
    for (int i = 0; i < playersCount; i++) {
        if (p[i] > 0) {
            addPlayerToGame(players[p[i]], roomNumber, freePlayersOfType);
        }
    }
    for (int i = 0; i < playersCount; i++) {
        if (p[i] < 0) {
            int t = (-1) * p[i];
            int number = getPlayerNumberWithType(players, t);
            addPlayerToGame(players[number], roomNumber, freePlayersOfType);
        }
    }
}

Proposition **createPropositionsList(const char *name, int size) {
    Proposition **list = (Proposition **) malloc((size + 2) * sizeof(Proposition *));
    for (int i = 0; i < size + 2; i++) {
        char *cellName = catStringAndNumber(name, i);
        list[i] = (Proposition *) createMemory(cellName, sizeof(Proposition));
        char *playersName = catTwoStrings(cellName, PROPOSITIONS_PLAYERS_SUFIX);
        list[i]->players = (int *) createMemory(playersName, size * sizeof(int)); // size is also all players count
        free(playersName);
        free(cellName);
        list[i]->playersCount = 0;
        list[i]->initiator = i;
        list[i]->active = 0;
    }
    list[0]->prev = 0;
    list[0]->next = size + 1;
    list[size + 1]->prev = 0;
    list[size + 1]->next = size + 1;
    return list;
}

void deletePropositionsList(const char *name, int size) {
    for (int i = 0; i < size + 2; i++) {
        char *cellName = catStringAndNumber(name, i);
        deleteMemory(cellName);
        char *playersName = catTwoStrings(cellName, PROPOSITIONS_PLAYERS_SUFIX);
        deleteMemory(playersName);
        free(playersName);
        free(cellName);
    }
}

void closePropositionsList(Proposition **propositions, int size) {
    for (int i = 0; i < size + 2; i++) {
        closeMemory(propositions[i]->players, size * sizeof(int));
        closeMemory(propositions[i], sizeof(Proposition));
    }
    free(propositions);
}

void addProposition(Proposition **propositions, Player **playersList, int allPlayersCount, int playerNumber,
        int roomType, int playersCount, const int *players) {
    int currentNumber = allPlayersCount;
    int prev = propositions[currentNumber]->prev;
    int next = currentNumber;
    propositions[prev]->next = playerNumber;
    propositions[next]->prev = playerNumber;
    propositions[playerNumber]->prev = prev;
    propositions[playerNumber]->next = next;
    propositions[playerNumber]->roomType = roomType;
    propositions[playerNumber]->playersCount = playersCount;
    closeMemory(propositions[playerNumber]->players, propositions[playerNumber]->playersCount * sizeof(int));
    for (int i = 0; i < playersCount; i++) {
        propositions[playerNumber]->players[i] = players[i];
    }
    propositions[playerNumber]->active = 1;
    playersList[playerNumber]->canPropose = 0;
    playersList[playerNumber]->propositions--;
}

void removeProposition(Proposition **propositions, int playerNumber) {
    int prev = propositions[playerNumber]->prev;
    int next = propositions[playerNumber]->next;
    propositions[prev]->next = next;
    propositions[next]->prev = prev;
    propositions[playerNumber]->active = 0;
}

int findAndExecuteProposition(Proposition **propositions, Room **freeRooms, Player **players,
        int *freePlayersOfType, int *pendingGames) {
    int currentNumber = propositions[0]->next;
    while (!isLastProposition(propositions[currentNumber]) &&
    !canExecute(propositions[currentNumber], freeRooms, players, freePlayersOfType))
        currentNumber = propositions[currentNumber]->next;
    if (isLastProposition(propositions[currentNumber])) return 0;
    execute(propositions[currentNumber], freeRooms, players, freePlayersOfType, pendingGames);
    return currentNumber;
}

/***************************************************************/

int main() {
    return 0;
}
