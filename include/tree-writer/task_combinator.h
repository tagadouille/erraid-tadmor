#include "tree-writer/task_creator.h"
#include "communication/request.h"
#include "types/timing.h"

/**
 * @brief Crée une tâche combinée en déplaçant les commandes des tâches
 *        existantes, puis en détruisant ces dernières.
 * @param timing Le timing pour la nouvelle tâche combinée.
 * @param composed La structure contenant les IDs des tâches à combiner.
 * @return L'ID de la nouvelle tâche en cas de succès, -1 en cas d'erreur.
 */
int64_t combine_and_destroy_tasks(const timing_t *timing, const composed_t *composed);