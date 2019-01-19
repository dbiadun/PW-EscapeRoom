//
// Created by dawid on 05.01.19.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include "helper.h"

void writeAboutExitingPlayer(int playerNumber, int gamesCount) {
    printf("Player %d left after %d game(s)\n", playerNumber, gamesCount);
}

int main() {
    Shared *shared = createMemory(SHARED_VARIABLES, sizeof(Shared));
    shared->typesCount = 'Z' - 'A' + 1;
    shared->waitingForPropositionsVerification = 0;
    shared->waitingForPropositionsAdding = 0;
    shared->waitingAfterPropositionsAdding = 0;
    shared->playersWithPropositions = 0;
    shared->pendingGames = 0;

    scanf("%d %d", &(shared->playersCount), &(shared->roomsCount));

    int playersCount = shared->playersCount;
    int roomsCount = shared->roomsCount;
    int typesCount = shared->typesCount;

    Room **freeRooms = createRoomsList(FREE_ROOMS, roomsCount);
    Player **freePlayers = createPlayersList(FREE_PLAYERS, playersCount);
    int **playersForPropositions = createPlayersForPropositions(OPEN_PROPOSITIONS, playersCount);
    Proposition **openPropositions = createPropositionsList(OPEN_PROPOSITIONS, playersCount);
    Type *types = createTypesArray(TYPES, typesCount);

///////////////////////////////////////////////////////////////////////////////

    semaphore mutex = createSemaphore(MUTEX_SEM, 1);
    semaphore forPropositionsVerification = createSemaphore(FOR_PROPOSITIONS_VERIFICATION_SEM, 0);
    semaphore forExitingPlayers = createSemaphore(FOR_EXITING_PLAYERS_SEM, 0);
    semaphore forPropositionsAdding = createSemaphore(FOR_PROPOSITIONS_ADDING_SEM, 0);
    semaphore afterPropositionsAdding = createSemaphore(AFTER_PROPOSITIONS_ADDING_SEM, 0);
    semaphore *forGame = createSemaphoresArray(FOR_GAME_SEM, 0, playersCount);
    semaphore *forPlayersInRoom = createSemaphoresArray(FOR_PLAYERS_IN_ROOM_SEM, 0, roomsCount);

///////////////////////////////////////////////////////////////////////////////

    for (int i = 1; i <= roomsCount; i++) {
        int nextChar = getchar();
        if (nextChar == '\n') nextChar = getchar();
        if (nextChar < 'A' || nextChar > 'Z') {
            fprintf(stderr, "Invalid Room type\n");
        }
        freeRooms[i]->type = typeNumberFromChar(nextChar);
        scanf("%d", &(freeRooms[i]->capacity));
        addRoom(freeRooms, i);
    }

///////////////////////////////////////////////////////////////////////////////

    for (int i = 1; i <= playersCount; i++) {
        char playerNumber[PLAYER_NUMBER_MAX_SIZE + 1];
        sprintf(playerNumber, "%d", i);

        switch (fork()) {
            case -1:
                perror("fork");
                _exit(1);
            case 0:
                execl("./player", "./player", playerNumber, NULL);
                _exit(0);
            default:
                break;
        }
    }


    for (int i = 0; i < playersCount; i++) {
        P(forExitingPlayers); // dziedziczenie sekcji krytycznej
        writeAboutExitingPlayer(shared->exitingPlayer, shared->gamesOfExitingPlayer);
        V(mutex);
    }

///////////////////////////////////////////////////////////////////////////////

    closeMemory(shared, sizeof(Shared));
    closeRoomsList(freeRooms, roomsCount);
    closePlayersList(freePlayers, playersCount);
    closePropositionsList(openPropositions, playersCount, playersForPropositions);
    closeTypesArray(types, typesCount);

    deleteMemory(SHARED_VARIABLES);
    deleteRoomsList(FREE_ROOMS, roomsCount);
    deletePlayersList(FREE_PLAYERS, playersCount);
    deletePropositionsList(OPEN_PROPOSITIONS, playersCount);
    deleteTypesArray(TYPES);

///////////////////////////////////////////////////////////////////////////////

    closeSemaphore(mutex);
    closeSemaphore(forPropositionsVerification);
    closeSemaphore(forExitingPlayers);
    closeSemaphore(forPropositionsAdding);
    closeSemaphore(afterPropositionsAdding);
    closeSemaphoresArray(forGame, playersCount);
    closeSemaphoresArray(forPlayersInRoom, roomsCount);

    deleteSemaphore(MUTEX_SEM);
    deleteSemaphore(FOR_PROPOSITIONS_VERIFICATION_SEM);
    deleteSemaphore(FOR_EXITING_PLAYERS_SEM);
    deleteSemaphore(FOR_PROPOSITIONS_ADDING_SEM);
    deleteSemaphore(AFTER_PROPOSITIONS_ADDING_SEM);
    deleteSemaphoresArray(FOR_GAME_SEM, playersCount);
    deleteSemaphoresArray(FOR_PLAYERS_IN_ROOM_SEM, roomsCount);

///////////////////////////////////////////////////////////////////////////////

    return 0;
}
