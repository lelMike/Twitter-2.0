#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>

#define SEMAPHORE_KEY 's'
#define MAX_KEY_LENGTH 255
#define MAX_USER_LENGTH 100
#define MAX_POST 500

struct record {
    char username[30];
    char post[500];
    int likes;
};

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

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Użycie: %s <shared_memory_key> <username>\n", argv[0]);
        return EXIT_FAILURE;
    }
    printf("Twitter 2.0 wita! (wersja B)\n");
    char SHARED_MEMORY_KEY[MAX_KEY_LENGTH];
    strncpy(SHARED_MEMORY_KEY, argv[1], sizeof(SHARED_MEMORY_KEY) - 1);
    SHARED_MEMORY_KEY[sizeof(SHARED_MEMORY_KEY) - 1] = '\0';

    char user[MAX_USER_LENGTH];
    strncpy(user, argv[2], sizeof(user) - 1);
    user[sizeof(user) - 1] = '\0';

    key_t key = ftok(SHARED_MEMORY_KEY, SEMAPHORE_KEY);
    if (key == -1) {
        perror("shared memory ftok");
        exit(EXIT_FAILURE);
    }

    int sem_flags = 0666;
    int semid = semget(key, 1, sem_flags);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(key, 0, 0);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    struct shmid_ds shmid_ds;
    if (shmctl(shmid, IPC_STAT, &shmid_ds) == -1) {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }

    int n = shmid_ds.shm_segsz / sizeof(struct record);

    struct record *shared_data = (struct record *)shmat(shmid, NULL, 0);
    if (shared_data == (struct record *)(-1)) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    int i = 0; int free_slots = n;
    waitSemaphore(semid);
    for(i; i < n; i++){
        if(shared_data[i].likes != -1){
            if(free_slots == n){
                printf("Istniejace wpisy\n");
            }
            printf("    %d.  %s [Autor: %s, Polubienia: %d]\n", n-free_slots+1, shared_data[i].post, shared_data[i].username, shared_data[i].likes);
            free_slots--;
        }
    }
    freeSemaphore(semid);

    printf("[Wolnych %d wpisow (na %d)]\n", free_slots, n);
    printf("Podaj akcje (N)owy wpis, (L)ike\n"); char choice;
    scanf("%c", &choice);

    if(choice == 'N'){
        i = 0; free_slots = n; int first_free_slot = 0;
        waitSemaphore(semid);
        for(i; i < n; i++) {
            if (shared_data[i].likes == -1) {
                if(free_slots == n){
                    first_free_slot = i;
                }
                free_slots--;
            }
        }
        if(free_slots != n){
            printf("Napisz co ci chodzi po glowie:\n");
            char text[MAX_POST]; getchar();
            fgets(text, sizeof(text), stdin);
            size_t len = strlen(text);
            if (len > 0 && text[len - 1] == '\n') {
                text[len - 1] = '\0';
            }
            strncpy(shared_data[first_free_slot].username, user, sizeof(shared_data[first_free_slot].username) - 1);
            strncpy(shared_data[first_free_slot].post, text, sizeof(shared_data[first_free_slot].post) - 1);
            shared_data[first_free_slot].likes = 0;
            freeSemaphore(semid);
        }
        else{
            freeSemaphore(semid);
            printf("Brak miejsca na nowe posty\n");
        }

    }
    else if(choice == 'L'){
        printf("Ktory wpis chcesz polubic\n");
        int num; scanf("%d", &num);
        if(num > n){
            printf("Nie istnieje taki post\n");
        }
        else{
            waitSemaphore(semid);
            if(shared_data[num-1].likes == -1){
                printf("Nie istnieje taki post\n");
            }
            else{
                shared_data[num-1].likes++;
                printf("Dodałem like\n");
            }
            freeSemaphore(semid);
        }
    }
    else{
        printf("Bledne polecenie\n");
    }
    printf("Dziekuje za skorzystanie z aplikacji Twitter 2.0\n");

    return 0;
}
