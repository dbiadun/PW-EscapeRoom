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

int main() {
    int playersCount, roomsCount, memory, i;

    scanf("%d %d", &playersCount, &roomsCount);

//    memory = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
//    if (memory == -1) {
//        perror("shm_open");
//        exit(1);
//    }
//
//    if (ftruncate(memory, roomsCount * sizeof(Room))) {
//        perror("ftruncate");
//        exit(1);
//    }

//    Room *rooms = (Room*) malloc(roomsCount * sizeof(Room));
//
//    for (i = 0; i < roomsCount; i++) {
//        rooms[i].type = getchar();
//        if (rooms[i].type == '\n') rooms[i].type = getchar();
//        if (rooms[i].type < 'A' || rooms[i].type > 'Z') {
//            fprintf(stderr, "Invalid Room type\n");
//        }
//
//        scanf("%d", &(rooms[i].maxCount));
//    }

    for (i = 1; i <= playersCount; i++) {
        char playerNumber[PLAYER_NUMBER_MAX_SIZE];
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


    ////////////////////////////////////////////////////////////////////
    int exitingPlayer;
    int gamesOfExitingPlayer;

    semaphore forExitingPlayers = 0;
    semaphore mutex;


    for (i = 0; i < playersCount; i++) {
        P(forExitingPlayers); // dziedziczenie sekcji krytycznej
        writeDataOut();
        V(mutex);
    }

    close(memory);
    shm_unlink(SHM_NAME);
    return 0;
}
