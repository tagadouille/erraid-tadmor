#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "types/time_exitcode.h"
#include "types/timing.h"
#include "types/argument.h"

int main() {
    timing_t t;

    // Étape 1 — Création manuelle d'un timing
    // Exemple : minutes 0–10, heures 8 et 18, jours lundi à vendredi
    t.minutes = 0;
    for (int i = 0; i <= 10; i++)
        t.minutes |= (1ULL << i);  // active bits 0–10
    t.hours = (1U << 8) | (1U << 18); // 8h et 18h
    t.daysofweek = 0;
    for (int i = 1; i <= 5; i++)
        t.daysofweek |= (1U << i);  // lundi(1) à vendredi(5)

    printf("=== Timing initial ===\n");

    // Étape 2 — Écriture dans un fichier binaire
    if (!timing_write("test_timing.bin", &t)) {
        perror("timing_write");
        return 1;
    }
    printf("\n Timing écrit dans test_timing.bin\n");

    // Étape 3 — Lecture depuis le fichier
    timing_t t2;
    if (!timing_read("test_timing.bin", &t2)) {
        perror("timing_read");
        return 1;
    }
    printf("\n=== Timing relu depuis le fichier ===\n");


    // Étape 4 — Vérification : moment actuel
    printf("\n=== Vérification de l'heure actuelle ===\n");
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    printf("Il est actuellement %02d:%02d, jour de la semaine : %d\n",
           tm_now->tm_hour, tm_now->tm_min, tm_now->tm_wday);

    if (timing_match_now(&t2))
        printf(" C'est le moment d'exécuter la tâche !\n");
    else
        printf(" Ce n'est PAS encore le moment.\n");

    
    time_exitcode_t record;

    record.time = time(NULL);
    record.exitcode = 0; // exemple : succès

    time_exitcode_print(&record);


    // Test de arguments_parse
    unsigned char buf[] = {
    0x00,0x00,0x00,0x03,
    0x00,0x00,0x00,0x04,'t','e','s','t',
    0x00,0x00,0x00,0x03,'a','r','g',
    0x00,0x00,0x00,0x05,'v','a','l','u','e'
    };

    char* parsed = arguments_parse((const char*)buf, sizeof(buf));
    if (parsed) {
        printf("Parsed arguments: %s\n", parsed);
        free(parsed);
    } else {
        printf("Failed to parse arguments\n");
    }
    printf("Done.\n");

    return 0;
}