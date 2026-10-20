#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_RECORDS 100
#define MSG_SIZE 256

typedef struct {
    int id;
    char *name;
    int *scores;
} Record;

Record *create_record(int id, const char *name, int num_scores) {
    Record *r = malloc(sizeof(Record));

    r->id = id;

    r->name = malloc(strlen(name) + 1);
    strcpy(r->name, name);

    r->scores = malloc(sizeof(int) * num_scores);

    for (int i = 0; i < num_scores; i++) {
        r->scores[i] = id * 10 + i;
    }

    return r;
}

/* Fixed: Giai phong day du bo nho */
void delete_record(Record *r) {
    free(r->name);
    free(r->scores);
    free(r);
}

/* Fixed: Giai phong buffer sau khi su dung */
void process_messages(int count) {
    for (int i = 0; i < count; i++) {
        char *buf = malloc(MSG_SIZE);

        snprintf(buf, MSG_SIZE,
                 "Message #%d: data payload here", i);

        if (i % 10 == 0) {
            printf("Processed: %s\n", buf);
        }

        free(buf);
    }
}

int main(void) {
    printf("=== Leaky App - LAB03 Memory Monitor ===\n");

    Record **records = malloc(sizeof(Record*) * NUM_RECORDS);

    for (int i = 0; i < NUM_RECORDS; i++) {
        records[i] = create_record(i, "EmbeddedLinuxStudent", 5);
    }

    for (int i = 0; i < NUM_RECORDS; i++) {
        delete_record(records[i]);
    }

    free(records);

    process_messages(50);

    printf("Program finished! Waiting 30 seconds...\n");

    /* Keep process alive for /proc memory monitoring */
    sleep(30);

    return 0;
}
