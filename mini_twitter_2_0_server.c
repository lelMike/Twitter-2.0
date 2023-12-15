#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define SEMAPHORE_KEY 's'
#define MAX_KEY_LENGTH 255

volatile sig_atomic_t signal_received = 0;

struct record {
    char username[30];
    char post[500];
    int likes;
};

void sleepSeconds(long nanoseconds) {
    struct timespec req;

    req.tv_sec = (nanoseconds*100000000) / 1000000000;
    req.tv_nsec = (nanoseconds*100000000) % 1000000000;

    nanosleep(&req, NULL);
}

void waitSemaphore(int semid) {
    struct sembuf sem_wait = {
            .sem_num = 0,
            .sem_op = -1,
            .sem_flg = 0
    };
    semop(semid, &sem_wait, 1);
}

void freeSemaphore(int semid) {
    struct sembuf sem_signal = {
            .sem_num = 0,
            .sem_op = 1,
            .sem_flg = 0
    };
    semop(semid, &sem_signal, 1);
}

void printRecord(struct record *rec) {
    printf("[%s]: %s [Polubienia: %d]\n", rec->username, rec->post, rec->likes);
}

void handleSIGTSTP(int signum) {
    signal_received = 1;
}

void handleSIGINT(int signum) {
    signal_received = 2;
}

void initializeCell(struct record *rec) {
    rec->username[0] = '\0';
    rec->post[0] = '\0';
    rec->likes = -1;
}

void cleanup(int shmid, int semid, struct record *shared_data) {
    printf("(odlaczenie: ");
    if (shmdt(shared_data) == -1) {
        perror("shmdt");
    }

    printf("OK, usuniecie pamieci wspoldzielonej: ");
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    }

    printf("OK, usuniecie semafora: ");
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl");
    }
    printf("OK)\n"); fflush(stdout);
}

int main(int argc, char **argv) {
    printf("[Serwer]: Twitter 2.0 (wersja B)\n"); fflush(stdout); sleepSeconds(6);

    if (signal(SIGTSTP, handleSIGTSTP) == SIG_ERR) {
        perror("Signal");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGINT, handleSIGINT) == SIG_ERR) {
        perror("Signal");
        exit(EXIT_FAILURE);
    }

    if (argc != 3) {
        fprintf(stderr, "Uzycie: %s <plik> <ile postów>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char SHARED_MEMORY_KEY[MAX_KEY_LENGTH];
    strncpy(SHARED_MEMORY_KEY, argv[1], sizeof(SHARED_MEMORY_KEY) - 1);
    SHARED_MEMORY_KEY[sizeof(SHARED_MEMORY_KEY) - 1] = '\0';

    int n;
    char *endptr;
    n = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0') {
        perror("Error konwersji do liczby");
        exit(EXIT_FAILURE);
    }

    printf("[Serwer]: Tworze klucz pamieci wspoldzielonej na podstawie pliku %s", SHARED_MEMORY_KEY); fflush(stdout); sleepSeconds(6); printf("."); fflush(stdout); sleepSeconds(6); printf("."); fflush(stdout); sleepSeconds(6); printf("."); fflush(stdout); sleepSeconds(3);
    key_t key = ftok(SHARED_MEMORY_KEY, SEMAPHORE_KEY);
    if (key == -1) {
        perror("shared memory ftok");
        exit(EXIT_FAILURE);
    }
    printf(" OK (klucz: %d)\n", key);

    printf("[Serwer]: Tworze segment pamieci wspoldzielonej na %d wpisow po %ldb", n, sizeof(struct record)); fflush(stdout); sleepSeconds(6); printf("."); fflush(stdout); sleepSeconds(6); printf("."); fflush(stdout); sleepSeconds(6); printf(".\n"); fflush(stdout); sleepSeconds(3);
    int shmid = shmget(key, n * sizeof(struct record), IPC_CREAT | IPC_CREAT | 0666);
    if (shmid == -1) {
        if (errno == EEXIST){
            perror("Istnieje pamiec wspoldzielona");
            exit(EXIT_FAILURE);
        }
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    printf("          OK (id: %d, rozmiar: %ldb)\n", shmid, n*sizeof(struct record));

    printf("[Serwer]: Dolaczam pamiec wspolna"); fflush(stdout); sleepSeconds(6); printf("."); fflush(stdout); sleepSeconds(6); printf("."); fflush(stdout); sleepSeconds(6); printf("."); fflush(stdout); sleepSeconds(3);
    struct record *shared_data = (struct record *)shmat(shmid, NULL, 0);
    printf(" OK (adres: %p)\n", &shared_data);

    printf("[Serwer]: Tworze semafor"); fflush(stdout); sleepSeconds(6); printf(".");
    int sem_flags = IPC_CREAT | IPC_CREAT | 0666;
    int semid = semget(key, 1, sem_flags);
    if (semid == -1) {
        if(errno == EEXIST){
            printf("([BLAD] Semafor juz istnieje\nodlaczenie pamieci wspoldzielonej: ");
            if (shmdt(shared_data) == -1) {
                perror("shmdt");
            }

            printf("OK, usuniecie pamieci wspoldzielonej: ");
            if (shmctl(shmid, IPC_RMID, NULL) == -1) {
                perror("shmctl");
            }
            printf("OK\n");
            perror("Semafor juz istnieje");
            exit(EXIT_FAILURE);
        }
        perror("semget");
        exit(EXIT_FAILURE);
    }
    fflush(stdout); sleepSeconds(6); printf(".");

    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;
    arg.val = 1;
    fflush(stdout); sleepSeconds(6); printf("."); fflush(stdout); sleepSeconds(3);
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    printf("          OK (id: %d)\n", semid); fflush(stdout); sleepSeconds(3);

    int i = 0;
    for (i; i < n; i++) {
        waitSemaphore(semid);
        initializeCell(&shared_data[i]);
        freeSemaphore(semid);
    }

    printf("[Serwer]: nacisnij Crtl^Z by wyswietlic stan serwisu\n"); fflush(stdout); sleepSeconds(3);
    printf("[Serwer]: nacisnij Crtl^C by zakonczyc program\n"); fflush(stdout); sleepSeconds(3);
    while (1) {
        if (signal_received == 1) {
            // SIGTSTP ctrl + z
            i = 0; char twitter_handle = 0; char none_achieved = 1;
            waitSemaphore(semid);
            for (i; i < n; i++) {
                if (shared_data[i].likes != -1) {
                    if (twitter_handle == 0){
                        printf("___________  Twitter 2.0:  ___________\n"); twitter_handle = 1;
                    }
                    printRecord(&shared_data[i]);
                    none_achieved = 0;
                }
            }
            freeSemaphore(semid);
            if(none_achieved){
                printf("Brak wpisow\n");
            }
            signal_received = 0;
        } else if (signal_received == 2) {
            // SIGINT ctrl + c
            printf("[Serwer]: Dostałem SIGINT => koncze i sprzatam ");
            fflush(stdout); sleepSeconds(6); printf("."); fflush(stdout); sleepSeconds(6); printf("."); fflush(stdout); sleepSeconds(6); printf(". "); fflush(stdout); sleepSeconds(3);
            cleanup(shmid, semid, shared_data);
            exit(EXIT_SUCCESS);
        }
        sleepSeconds(0.1);
    }

    return 0;
}
