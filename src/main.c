#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "../include/types/time_exitcode.h"
#include "../include/types/timing.h"

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
    timing_print(&t);

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
    timing_print(&t2);

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
    return 0;


    time_exitcode_t record;

    record.time = time(NULL);
    record.exitcode = 0; // exemple : succès

    if (!time_exitcode_append("times-exitcodes", &record)) {
        perror("append");
        return 1;
    }

    time_exitcode_show("times-exitcodes");
    return 0;
}