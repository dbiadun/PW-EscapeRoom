//
// Created by dawid on 05.01.19.
//


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "helper.h"

#define FILE_PREFIX "player"
#define ERROR -1
#define OK 0
#define LINE_END 1
#define FILE_END 2

/************************************ FILES ***********************************/

char *fileNameIn(int playerNumber) {
    char *name = (char *) malloc(strlen(FILE_PREFIX) + 5 + PLAYER_NUMBER_MAX_SIZE);
    sprintf(name, "%s-%d.in", FILE_PREFIX, playerNumber);
    return name;
}

char *fileNameOut(int playerNumber) {
    char *name = (char *) malloc(strlen(FILE_PREFIX) + 6 + PLAYER_NUMBER_MAX_SIZE);
    sprintf(name, "%s-%d.out", FILE_PREFIX, playerNumber);
    return name;
}

int stringToNumber(const int *string) {
    int ret = 0;
    for (int i = 0; string[i] != '\0'; i++) {
        ret *= 10;
        ret += string[i] - '0';
    }
    return ret;
}

int skipLine(FILE *f) {
    int c = ' ';
    while (c != '\n' && c != EOF) c = fgetc(f);
    switch (c) {
        case '\n': return LINE_END;
        case EOF: return FILE_END;
        default: return ERROR;
    }
}

int readPlayer(FILE *f, int *buffer) {
    int i = -1;
    do {
        i++;
        buffer[i] = fgetc(f);
    } while (buffer[i] != ' ' && buffer[i] != '\n' && buffer[i] != EOF);
    int aux = buffer[i];
    buffer[i] = '\0';
    switch (aux) {
        case ' ': return OK;
        case '\n': return LINE_END;
        case EOF: return FILE_END;
        default: return ERROR;
    }
}

int readTypeOfRoom(FILE *f, int *buffer) {
    buffer[0] = fgetc(f);
    buffer[1] = '\0';
    if (buffer[0] == EOF) return ERROR;
    switch (fgetc(f)) {
        case ' ': return OK;
        case '\n': return LINE_END;
        case EOF: return FILE_END;
        default: return ERROR;
    }
}

int readLine(FILE *f, int *buffer, int *players, int *typeNumber, int *playersCount, int maxPlayersCount,
        int initiator) {
    *playersCount = 0;
    int ret = readTypeOfRoom(f, buffer);
    *typeNumber = typeNumberFromChar(buffer[0]);
    if (ret != OK) return ret;

    int i = 0;
    do {
        ret = readPlayer(f, buffer);
        if (buffer[0] >= 'A' && buffer[0] <= 'Z') {
            players[i] = typeNumberToPlayerNumber(typeNumberFromChar(buffer[0]));
        } else {
            players[i] = stringToNumber(buffer);
        }
        i++;
    } while (ret == OK && i < maxPlayersCount - 1);
    players[i] = initiator;
    *playersCount = i + 1;
    return ret;
}

int readTypeOfPlayer(FILE *f, int *buffer) {
    buffer[0] = fgetc(f);
    buffer[1] = '\0';
    if (fgetc(f) == EOF) return FILE_END;
    return LINE_END;
}

int countPropositions(FILE *f) {
    int count;
    int ret = skipLine(f);
    for (count = 0; ret != FILE_END; count++) ret = skipLine(f);
    return count;
}

void invalidGame(FILE *f, FILE *errorsFile) {
    fprintf(f, "Invalid game \"");
    int aux = fgetc(errorsFile);
    while (aux != '\n' && aux != EOF) {
        fputc(aux, f);
        aux = fgetc(errorsFile);
    }
    fprintf(f, "\"\n");
}

void gameStart(FILE *f, int initiator, int roomNumber, Player **players, int playersCount) {
    fprintf(f, "Game defined by %d is going to start: room %d, players (", initiator, roomNumber);
    int j = 0;
    for (int i = 1; i < playersCount + 1; i++) {
        if (players[i]->currentRoom == roomNumber) {
            j++;
            if (j == 1) fprintf(f, "%d", players[i]->number);
            else fprintf(f, ", %d", players[i]->number);
        }
    }
    fprintf(f, ")\n");
}

void enteredRoom(FILE *f, int initiator, int roomNumber, Player **players, int playersCount) {
    fprintf(f, "Entered room %d, game defined by %d, waiting for players (", roomNumber, initiator);
    int j = 0;
    for (int i = 1; i < playersCount + 1; i++) {
        if (players[i]->currentRoom == roomNumber && !players[i]->waitingInside) {
            j++;
            if (j == 1) fprintf(f, "%d", players[i]->number);
            else fprintf(f, ", %d", players[i]->number);
        }
    }
    fprintf(f, ")\n");
}

void leftRoom(FILE *f, int roomNumber) {
    fprintf(f, "Left room %d\n", roomNumber);
}

/******************************************************************************/

void goThroughBarrier(semaphore mutex, semaphore barrierSem, int *counter, int all, int inCriticalSection) {
    if (!inCriticalSection) P(mutex);
    if (*counter < all - 1) {
        (*counter)++;
        V(mutex);
        P(barrierSem); // inherits critical session
        (*counter)--;
        if (*counter > 0) V(barrierSem); // passes critical section
        else V(mutex);
    } else {
        V(barrierSem); // passes critical section
    }
}

void tryToExecuteProposition(Proposition **openPropositions, int **playersForPropositions, Room **freeRooms,
        Player **freePlayers, Type *types, Shared *shared, semaphore mutex, semaphore *forGame, int *canContinue,
        int playerNumber, int playerType, FILE *outFile) { // inherits critical section
    int proposition = findAndExecuteProposition(openPropositions, freeRooms, freePlayers, types,
                                                &(shared->pendingGames), playersForPropositions, forGame);
    if (proposition != 0) {
        gameStart(outFile, proposition, freePlayers[proposition]->currentRoom, freePlayers,
                shared->playersCount);
    }
    if (proposition == 0 || !freePlayers[playerNumber]->busy) {
        if (shared->playersWithPropositions > 0 || freePlayers[playerNumber]->propositions > 0
            || types[playerType].propositions > 0) {
            V(mutex);
            P(forGame[playerNumber]);
            P(mutex);
            if (shared->playersWithPropositions <= 0 && freePlayers[playerNumber]->propositions <= 0
                  && types[playerType].propositions <= 0 && !freePlayers[playerNumber]->busy) {
                *canContinue = 0;
            }
        } else {
            if (shared->pendingGames == 0) {
                int count = shared->playersCount;
                for (int i = 1; i <= count; i++) {
                    V(forGame[i]);
                }
            }
            *canContinue = 0;
        }
    } else {
        V(mutex);
        P(forGame[playerNumber]);
        P(mutex);
    }
}

int main(int argc, char *argv[]) {
    int playerNumber;
    int playerType;
    int numberOfGames = 0;

    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments\n");
        _exit(1);
    }

    sscanf(argv[1], "%d", &playerNumber);

    ///////////////////////////////////////////////////////////////////////////

    Shared *shared = getMemory(SHARED_VARIABLES, sizeof(Shared));
    Room **freeRooms = getRoomsList(FREE_ROOMS, shared->roomsCount);
    Player **freePlayers = getPlayersList(FREE_PLAYERS, shared->playersCount);
    int **playersForPropositions = getPlayersForPropositions(OPEN_PROPOSITIONS, shared->playersCount);
    Proposition **openPropositions = getPropositionsList(OPEN_PROPOSITIONS, shared->playersCount);
    Type *types = getTypesArray(TYPES, shared->typesCount);

    ///////////////////////////////////////////////////////////////////////////

    semaphore mutex = getSemaphore(MUTEX_SEM);
    semaphore forPropositionsVerification = getSemaphore(FOR_PROPOSITIONS_VERIFICATION_SEM);
    semaphore forExitingPlayers = getSemaphore(FOR_EXITING_PLAYERS_SEM);
    semaphore forPropositionsAdding = getSemaphore(FOR_PROPOSITIONS_ADDING_SEM);
    semaphore afterPropositionsAdding = getSemaphore(AFTER_PROPOSITIONS_ADDING_SEM);
    semaphore *forGame = getSemaphoresArray(FOR_GAME_SEM, shared->playersCount);
    semaphore *forPlayersInRoom = getSemaphoresArray(FOR_PLAYERS_IN_ROOM_SEM, shared->roomsCount);

    ///////////////////////////////////////////////////////////////////////////

    int buffer[PLAYER_NUMBER_MAX_SIZE + 1];
    char *inputFileName = fileNameIn(playerNumber);
    char *outputFileName = fileNameOut(playerNumber);

    FILE *errorsFile = fopen(inputFileName, "r");
    FILE *inputFile = fopen(inputFileName, "r");
    FILE *outputFile = fopen(outputFileName, "w");
    int maxPropositionsCount = countPropositions(inputFile);
    fclose(inputFile);
    inputFile = fopen(inputFileName, "r");

    P(mutex);
    int propositionsCount = 0;
    int currentProposition = 0;
    int *roomsTypes = (int *) malloc(maxPropositionsCount * sizeof(int));
    int *playersInPropositions = (int *) malloc(maxPropositionsCount * sizeof(int));
    int **propositions = (int **) malloc(maxPropositionsCount * sizeof(int *));
    int players[shared->playersCount];
    int playersCount;
    int typeNumber;
    int ret;

    ret = readTypeOfPlayer(inputFile, buffer);
    skipLine(errorsFile);
    playerType = typeNumberFromChar(buffer[0]);
    freePlayers[playerNumber]->type = playerType;
    types[playerType].freePlayers++;

    goThroughBarrier(mutex, forPropositionsVerification, &(shared->waitingForPropositionsVerification),
            shared->playersCount, 1);

    P(mutex);
    while (ret != FILE_END && ret != ERROR) {
        ret = readLine(inputFile, buffer, players, &typeNumber, &playersCount, shared->playersCount, playerNumber);
        if (ret == FILE_END || ret == LINE_END) {
            Proposition proposition;
            proposition.roomType = typeNumber;
            proposition.playersCount = playersCount;
            if (playersCount > 0 && canExecute(&proposition, freeRooms, freePlayers, types, players)) {
                roomsTypes[propositionsCount] = typeNumber;
                playersInPropositions[propositionsCount] = playersCount;
                propositions[propositionsCount] = (int *) malloc(playersCount * sizeof(int));
                for (int i = 0; i < playersCount; i++) propositions[propositionsCount][i] = players[i];

                skipLine(errorsFile);
                propositionsCount++;
            } else {
                invalidGame(outputFile, errorsFile);
            }
        } else if (ret == OK) {
            skipLine(inputFile);
            invalidGame(outputFile, errorsFile);
        }
    }
    if (propositionsCount > 0) shared->playersWithPropositions++;
    V(mutex);

    ///////////////////////////////////////////////////////////////////////////

    goThroughBarrier(mutex, forPropositionsAdding, &(shared->waitingForPropositionsAdding), shared->playersCount, 0);
    P(mutex);
    if (currentProposition < propositionsCount) {
        addProposition(openPropositions, freePlayers, shared, shared->playersCount, playerNumber,
                roomsTypes[currentProposition], playersInPropositions[currentProposition],
                propositions[currentProposition], types, playersForPropositions, &currentProposition, propositionsCount);
    }
    goThroughBarrier(mutex, afterPropositionsAdding, &(shared->waitingAfterPropositionsAdding), shared->playersCount, 1);

    int canContinue = 1;
    P(mutex);

    if (!freePlayers[playerNumber]->busy) {
        tryToExecuteProposition(openPropositions, playersForPropositions, freeRooms, freePlayers, types, shared, mutex,
                forGame, &canContinue, playerNumber, playerType, outputFile);
    }

    while (canContinue) {
        int roomNumber = freePlayers[playerNumber]->currentRoom;
        Room *room = getRoomFromNumber(freeRooms, roomNumber);
        freePlayers[playerNumber]->waitingInside = 1;
        room->playersPlayingInside++;
        enteredRoom(outputFile, room->initiator, roomNumber, freePlayers, shared->playersCount);
        goThroughBarrier(mutex, forPlayersInRoom[roomNumber], &(room->playersWaitingInside), room->neededPlayers, 1);

        P(mutex);
        leftRoom(outputFile, roomNumber);
        numberOfGames++;
        freePlayers[playerNumber]->busy = 0;
        freePlayers[playerNumber]->waitingInside = 0;
        freePlayers[playerNumber]->currentRoom = 0;
        types[playerType].freePlayers++;
        room->playersPlayingInside--;
        addPlayer(freePlayers, shared->playersCount, playerNumber);
        if (roomEmpty(room)) {
            freeRoom(freeRooms, freePlayers, roomNumber, &(shared->pendingGames), forGame);
        }
        while (!freePlayers[playerNumber]->busy && canContinue) {
            if (freePlayers[playerNumber]->canPropose && currentProposition < propositionsCount) {
                addProposition(openPropositions, freePlayers, shared, shared->playersCount, playerNumber,
                        roomsTypes[currentProposition], playersInPropositions[currentProposition],
                        propositions[currentProposition], types, playersForPropositions, &currentProposition,
                        propositionsCount);
            }
            tryToExecuteProposition(openPropositions, playersForPropositions, freeRooms, freePlayers, types, shared,
                    mutex, forGame, &canContinue, playerNumber, playerType, outputFile);
        }
    }


    shared->exitingPlayer = playerNumber;
    shared->gamesOfExitingPlayer = numberOfGames;
    V(forExitingPlayers); // passes critical section

    ///////////////////////////////////////////////////////////////////////////

    free(roomsTypes);
    free(playersInPropositions);
    for (int i = 0; i < propositionsCount; i++) free(propositions[i]);
    free(propositions);

    free(inputFileName);
    free(outputFileName);
    fclose(errorsFile);
    fclose(inputFile);
    fclose(outputFile);

    ///////////////////////////////////////////////////////////////////////////

    closeMemory(shared, sizeof(Shared));
    closeRoomsList(freeRooms, shared->roomsCount);
    closePlayersList(freePlayers, shared->playersCount);
    closePropositionsList(openPropositions, shared->playersCount, playersForPropositions);
    closeTypesArray(types, shared->typesCount);

    ///////////////////////////////////////////////////////////////////////////

    closeSemaphore(mutex);
    closeSemaphore(forPropositionsVerification);
    closeSemaphore(forExitingPlayers);
    closeSemaphore(forPropositionsAdding);
    closeSemaphore(afterPropositionsAdding);
    closeSemaphoresArray(forGame, shared->playersCount);
    closeSemaphoresArray(forPlayersInRoom, shared->roomsCount);

    ///////////////////////////////////////////////////////////////////////////

    return 0;

}
