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
    semaphore s = sem_open(name, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR, value);
    if (s == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }
    return s;
}

semaphore *createSemaphoresArray(const char *name, unsigned int value, int size) {
    semaphore *sArray = (semaphore *) malloc((size + 2) * sizeof(semaphore));
    for (int i = 0; i < size + 2; i++) {
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

void deleteSemaphoresArray(const char *name, int size) {
    for (int i = 0; i < size + 2; i++) {
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

semaphore *getSemaphoresArray(const char *name, int size) {
    semaphore *sArray = (semaphore *) malloc((size + 2) * sizeof(semaphore));
    for (int i = 0; i < size + 2; i++) {
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

void closeSemaphoresArray(semaphore *s, int size) {
    for (int i = 0; i < size + 2; i++) {
        closeSemaphore(s[i]);
    }
    free(s);
}

/************************************************************/


/**************************** TYPES ****************************/

Type *createTypesArray(const char *name, int size) {
    Type *types = (Type *) createMemory(name, (size + 1) * sizeof(Type));
    for (int i = 0; i < size + 1; i++) {
        types[i].number = i;
        types[i].propositions = 0;
        types[i].freePlayers = 0;
    }
    return types;
}

Type *getTypesArray(const char *name, int size) {
    return (Type *) getMemory(name, (size + 1) * sizeof(Type));
}

void deleteTypesArray(const char *name) {
    deleteMemory(name);
}

void closeTypesArray(Type *types, int size) {
    closeMemory(types, (size + 1) * sizeof(Type));
}

int typeNumberFromChar(int character) {
    return character - 'A' + 1;
}

int typeNumberFromPlayerNumber(int playerNumber) {
    return (-1) * playerNumber;
}

int typeNumberToPlayerNumber(int typeNumber) {
    return (-1) * typeNumber;
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

static void addPlayerToGame(Player *player, int roomNumber, Type *types, semaphore forGame) {
    player->busy = 1;
    player->currentRoom = roomNumber;
    types[player->type].freePlayers--;
    V(forGame);
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
        list[i]->currentRoom = 0;
        list[i]->waitingInside = 0;
    }
    list[0]->prev = 0;
    list[size + 1]->next = size + 1;
    return list;
}

Player **getPlayersList(const char *name, int size) {
    Player **list = (Player **) malloc((size + 2) * sizeof(Player *));
    for (int i = 0; i < size + 2; i++) {
        char *cellName = catStringAndNumber(name, i);
        list[i] = (Player *) getMemory(cellName, sizeof(Player));
        free(cellName);
    }
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
    int currentNumber = playersCount + 1;
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
        list[i]->neededPlayers = 0;
        list[i]->playersWaitingInside = 0;
        list[i]->playersPlayingInside = 0;
        list[i]->initiator = 0;
    }
    list[0]->prev = 0;
    list[0]->next = size + 1;
    list[size + 1]->prev = 0;
    list[size + 1]->next = size + 1;
    return list;
}

Room **getRoomsList(const char *name, int size) {
    Room **list = (Room **) malloc((size + 2) * sizeof(Room *));
    for (int i = 0; i < size + 2; i++) {
        char *cellName = catStringAndNumber(name, i);
        list[i] = (Room *) getMemory(cellName, sizeof(Room));
        free(cellName);
    }
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
    while (!isLastRoom(rooms[currentNumber]) && rooms[roomNumber]->capacity >= rooms[currentNumber]->capacity)
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

void freeRoom(Room **rooms, Player **players, int roomNumber, int *pendingGames, semaphore *forGame) {
    addRoom(rooms, roomNumber);
    (*pendingGames)--;
    players[rooms[roomNumber]->initiator]->canPropose = 1;
    V(forGame[rooms[roomNumber]->initiator]);
}

/***************************************************************/


/*********************** PROPOSITIONS **************************/

#define PROPOSITIONS_PLAYERS_SUFIX "players"

static int isLastProposition(Proposition *p) {
    return p->next == p->initiator;
}

static void execute(Proposition *proposition, Room **freeRooms, Player **players, Type *types, int *pendingGames,
        int *playersForProposition, semaphore *forGame) {
    int initiator = proposition->initiator;
    int playersCount = proposition->playersCount;
    int roomType = proposition->roomType;
    int roomNumber = getRoomNumberWithSizeAndType(freeRooms, playersCount, roomType);
    takeRoom(freeRooms, roomNumber, playersCount, initiator, pendingGames);

    int *p = playersForProposition;
    for (int i = 0; i < playersCount; i++) {
        if (p[i] > 0) {
            addPlayerToGame(players[p[i]], roomNumber, types, forGame[p[i]]);
            removePlayer(players, p[i]);
        }
    }
    for (int i = 0; i < playersCount; i++) {
        if (p[i] < 0) {
            int t = typeNumberFromPlayerNumber(p[i]);
            int number = getPlayerNumberWithType(players, t);
            addPlayerToGame(players[number], roomNumber, types, forGame[number]);
            removePlayer(players, number);
        }
    }
}

int **createPlayersForPropositions(const char *name, int playersCount) { // playersCount is also propositions count
    int **players = (int **) malloc((playersCount + 2) * sizeof(int *));
    for (int i = 0; i < playersCount + 2; i++) {
        char *cellName = catStringAndNumber(name, i);
        char *playersName = catTwoStrings(cellName, PROPOSITIONS_PLAYERS_SUFIX);
        players[i] = (int *) createMemory(playersName, playersCount * sizeof(int));
        free(playersName);
        free(cellName);
    }
    return players;
}

Proposition **createPropositionsList(const char *name, int size) {
    Proposition **list = (Proposition **) malloc((size + 2) * sizeof(Proposition *));
    for (int i = 0; i < size + 2; i++) {
        char *cellName = catStringAndNumber(name, i);
        list[i] = (Proposition *) createMemory(cellName, sizeof(Proposition));
        free(cellName);
        list[i]->playersCount = 0;
        list[i]->initiator = i;
        list[i]->active = 0;
        list[i]->roomType = 0;
    }
    list[0]->prev = 0;
    list[0]->next = size + 1;
    list[size + 1]->prev = 0;
    list[size + 1]->next = size + 1;
    return list;
}

int **getPlayersForPropositions(const char *name, int playersCount) {
    int **players = (int **) malloc((playersCount + 2) * sizeof(int *));
    for (int i = 0; i < playersCount + 2; i++) {
        char *cellName = catStringAndNumber(name, i);
        char *playersName = catTwoStrings(cellName, PROPOSITIONS_PLAYERS_SUFIX);
        players[i] = (int *) getMemory(playersName, playersCount * sizeof(int)); // size is also all players count
        free(playersName);
        free(cellName);
    }
    return players;
}

Proposition **getPropositionsList(const char *name, int size) {
    Proposition **list = (Proposition **) malloc((size + 2) * sizeof(Proposition *));
    for (int i = 0; i < size + 2; i++) {
        char *cellName = catStringAndNumber(name, i);
        list[i] = (Proposition *) getMemory(cellName, sizeof(Proposition));
        free(cellName);
    }
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

void closePropositionsList(Proposition **propositions, int size, int **players) {
    for (int i = 0; i < size + 2; i++) {
        closeMemory(players[i], size * sizeof(int));
        closeMemory(propositions[i], sizeof(Proposition));
    }
    free(players);
    free(propositions);
}

void addProposition(Proposition **propositions, Player **playersList, Shared *shared, int allPlayersCount,
        int playerNumber, int roomType, int playersCount, const int *players, Type *types,
        int **playersForPropositions, int *currentProposition, int propositionsCount) {
    int currentNumber = allPlayersCount + 1;
    int prev = propositions[currentNumber]->prev;
    int next = currentNumber;
    propositions[prev]->next = playerNumber;
    propositions[next]->prev = playerNumber;
    propositions[playerNumber]->prev = prev;
    propositions[playerNumber]->next = next;
    propositions[playerNumber]->roomType = roomType;
    propositions[playerNumber]->playersCount = playersCount;
    for (int i = 0; i < playersCount; i++) {
        playersForPropositions[playerNumber][i] = players[i];
        if (players[i] > 0) {
            playersList[players[i]]->propositions++;
        } else if (players[i] < 0) {
            int t = typeNumberFromPlayerNumber(players[i]);
            types[t].propositions++;
        }
    }
    propositions[playerNumber]->active = 1;
    playersList[playerNumber]->canPropose = 0;
    (*currentProposition)++;
    if (*currentProposition == propositionsCount) shared->playersWithPropositions--;
}

void removeProposition(Proposition **propositions, Player **playersList, int playerNumber, Type *types,
        int **playersForPropositions) {
    int prev = propositions[playerNumber]->prev;
    int next = propositions[playerNumber]->next;
    propositions[prev]->next = next;
    propositions[next]->prev = prev;
    propositions[playerNumber]->active = 0;
    int playersCount = propositions[playerNumber]->playersCount;
    int *players = playersForPropositions[playerNumber];
    for (int i = 0; i < playersCount; i++) {
        if (players[i] > 0) {
            playersList[players[i]]->propositions--;
        } else if (players[i] < 0) {
            int t = typeNumberFromPlayerNumber(players[i]);
            types[t].propositions--;
        }
    }
}

int canExecute(Proposition *proposition, Room **freeRooms, Player **players, Type *types, int *playersForProposition) {
    int playersCount = proposition->playersCount;
    int roomType = proposition->roomType;
    int roomNumber = getRoomNumberWithSizeAndType(freeRooms, playersCount, roomType);
    if (roomNumber == 0) return 0;
    int possible = 1;
    int *p = playersForProposition;
    for (int i = 0; i < playersCount; i++) {
        if (p[i] > 0) {
            Player *player = players[p[i]];
            types[player->type].freePlayers--;
            if (player->busy || types[player->type].freePlayers < 0) possible = 0;
            player->busy++;
        } else if (p[i] < 0) {
            int t = (-1) * p[i];
            types[t].freePlayers--;
            if (types[t].freePlayers < 0) possible = 0;
        }
    }
    for (int i = 0; i < playersCount; i++) {
        if (p[i] > 0) {
            Player *player = players[p[i]];
            types[player->type].freePlayers++;
            player->busy--;
        } else if (p[i] < 0) {
            int t = typeNumberFromPlayerNumber(p[i]);
            types[t].freePlayers++;
        }
    }
    return possible;
}

int findAndExecuteProposition(Proposition **propositions, Room **freeRooms, Player **players, Type *types,
        int *pendingGames, int **playersForPropositions, semaphore *forGame) {
    int currentNumber = propositions[0]->next;
    while (!isLastProposition(propositions[currentNumber]) &&
    !canExecute(propositions[currentNumber], freeRooms, players,
            types, playersForPropositions[currentNumber]))
        currentNumber = propositions[currentNumber]->next;
    if (isLastProposition(propositions[currentNumber])) return 0;
    execute(propositions[currentNumber], freeRooms, players, types,
            pendingGames, playersForPropositions[currentNumber], forGame);
    removeProposition(propositions, players, currentNumber, types, playersForPropositions);
    return currentNumber;
}

/***************************************************************/

