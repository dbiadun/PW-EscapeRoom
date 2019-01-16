//
// Created by dawid on 05.01.19.
//


#include <stdio.h>
#include <unistd.h>
#include "helper.h"

int main(int argc, char *argv[]) {
    int playerNumber;

    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments\n");
        _exit(1);
    }

    sscanf(argv[1], "%d", &playerNumber);

    printf("%d\n", playerNumber);

///////////////////////////////////////////////////////////////////

    int playerType;
    int playersCount;
    int typesCount = 'Z' - 'A' + 1;
    int roomsCount;
    int waitingForPropositionsAdding = 0;
    int waitingAfterPropositionsAdding = 0;
    int playersWithPropositions = playersCount;
    int playerBusy[playersCount];
    int propositionsForPlayer[playersCount];
    int propositionsForTypes[typesCount];
    int freePlayersOfType[typesCount];
    int canPropose[playersCount];
    Room *freeRooms[roomsCount];
    int pendingGames;
    int exitingPlayer;
    int gamesOfExitingPlayer;
    int numberOfGames = 0; // private

    semaphore mutex = 1;
    semaphore forExitingPlayers = 0;
    semaphore forPropositionsAdding = 0;
    semaphore afterPropositionsAdding = 0;
    semaphore forGame[playersCount] = 0;
    semaphore forPlayersInRoom[roomsCount] = 0;


    /*************** Wczytanie danych ****************/
    getData();


    /*************** Czekanie na dodawanie pierwszych propozycji *************/
    P(mutex);
    if (waitingForPropositionsAdding < playersCount - 1) {
        waitingForPropositionsAdding++;
        V(mutex);
        P(forPropositionsAdding); // dziedziczenie sekcji krytycznej
        waitingForPropositionsAdding--;
        if (waitingForPropositionsAdding > 0) V(forPropositionsAdding); // przekazanie sekcji krytycznej
        else V(mutex);
    } else {
        V(forPropositionsAdding); // przekazanie sekcji krytycznej
    }

    /**************** Dodawanie pierwszych propozycji *******************/
    P(mutex);
    addProposition();
    if (waitingAfterPropositionsAdding < playersCount - 1) {
        waitingAfterPropositionsAdding++;
        V(mutex);
        P(afterPropositionsAdding); // dziedziczenie sekcji krytycznej
        waitingAfterPropositionsAdding--;
        if (waitingAfterPropositionsAdding > 0) V(afterPropositionsAdding); // przekazanie sekcji krytycznej
        else V(mutex);
    } else {
        V(afterPropositionsAdding); // przekazanie sekcji krytycznej
    }

    /**************** Wejście do pokoju *******************/
    P(mutex);
    if (!playerBusy[playerNumber]) {
        int proposition = findProposition(); // powinno ustawić pokój dla propozycji
        if (proposition == 0) {
            if (playersWithPropositions > 0 || propositionsForPlayer[playerNumber] > 0
                || propositionsForTypes[playerType] > 0) {
                V(mutex);
                P(forGame[playerNumber]);
            } else {
                if (pendingGames == 0) {
                    int i;
                    int count = playersCount;
                    V(mutex);
                    for (i = 0; i < count; i++) {
                        V(forGame[i]);
                    }
                }
                endGame();
            }
        } else {
            executeProposition(proposition); // powinno zmienić playerBusy, freePlayersOfType, room->neededPlayers i room->playersWaitingInside
            P(forGame[playerNumber]);
        }
    }

    while (1) {
        int roomNumber = getRoomNumber(playerNumber);
        Room *room = getRoom(roomNumber);
        if (room->playersWaitingInside < room->neededPlayers - 1) {
            room->playersWaitingInside++;
            V(mutex);
            P(forPlayersInRoom[roomNumber]); // dziedziczenie sekcji krytycznej
            room->playersWaitingInside--;
            if (room->playersWaitingInside > 0) V(forPlayersInRoom[roomNumber]); // przekazanie sekcji krytycznej
            else V(mutex);
        } else {
            V(forPlayersInRoom[roomNumber]); // przekazanie sekcji krytycznej
        }


        /*************** Wyjście z pokoju *****************/
        P(mutex);
        playerBusy[playerNumber] = 0;
        if (lastOfTheRoom()) {
            canPropose[room->initiator] = 1;
            freeRoom(roomNumber);
        }
        int i;
        for (i = 0; !playerBusy[playerNumber]; i++) {
            if (canPropose[playerNumber]) {
                addProposition();
                canPropose[playerNumber] = 0;
            }
            int proposition = findProposition(); // powinno ustawić pokój dla propozycji
            if (proposition == 0) {
                if (playersWithPropositions > 0 || propositionsForPlayer[playerNumber] > 0
                    || propositionsForTypes[playerType] > 0) {
                    V(mutex);
                    P(forGame[playerNumber]);
                } else {
                    if (pendingGames == 0) {
                        int i;
                        int count = playersCount;
                        V(mutex);
                        for (i = 0; i < count; i++) {
                            V(forGame[i]);
                        }
                        P(mutex);
                    }
                    exitingPlayer = playerNumber;
                    gamesOfExitingPlayer = numberOfGames;
                    V(forExitingPlayers); // przekazanie sekcji krytycznej
                    _exit(0);
                }
            } else {
                executeProposition(proposition); // powinno zmienić playerBusy, freePlayersOfType, room->neededPlayers i room->playersWaitingInside
                P(forGame[playerNumber]);
            }
            V(mutex);
            P(forGame[playerNumber]);
            P(mutex);
        }
    }

}
