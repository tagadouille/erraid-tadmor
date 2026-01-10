# SY5-2025/2026-DAI-LIN-YOUNAN

## Description

Le projet est, comme dit dans l'énoncé, 'un outil d'automatisation de tâches périodiques à des moments spécifiés' sous Linux.
Et pour gérer les automatisations, on a séparé cet outil en deux composants principaux tadmor (client) et erraid (démon).

## Compilation
Le projet se compile via un fichier Makefile.
```bash
# Compiler le démon (erraid) et le client (tadmor)
make

# Nettoyer les fichiers compilés
make distclean
```

## Utilisation

### Le client 'tadmor'

Il doit être lancé après avoir lancé erraid. 

```bash
./tadmor
```

Le client tadmor propose plusieurs options comme demandé dans l'énoncé donné :

#### Création de tâche : 


* `-c [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] CMD [ARG_1] ... [ARG_N]` : création d'une tâche simple 

**Exemple : **

```bash
$ ./tadmor -c echo coucou         # création d'une tâche à exécuter à chaque minute
81
$ ./tadmor -c -m 0,3,6,9 date     # création d'une tâche avec horaires d'exécution précis
82
$ ./tadmor -c -n echo au revoir   # création d'une tâche "abstraite", sans exécution programmée
83
```

#### Combinaison de tâches : 

* `-s [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] TASKID_1 ... TASKID_N` :
   combinaison séquentielle des tâches d'identifiants `TASKID_1` à `TASKID_N`; la commande à exécuter est `( CMD_1 ; ... ; CMD_N )`

* `-p [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] TASKID_1 ... TASKID_N` :
   combinaison en _pipeline_ des tâches d'identifiants `TASKID_1` à `TASKID_N`; la commande à exécuter est `( CMD_1 | ... | CMD_N )`

* `-i [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] TASKID_1 TASKID_2 [TASKID_3]` : combinaison conditionnelle des tâches d'identifiants `TASKID_1`, `TASKID_2` et éventuellement `TASKID_3`; la commande à exécuter est `( if CMD_1 ; then CMD_2 ; else CMD_3 ; fi )`

* `-n` : en combinaison avec les précédentes, définition d'une tâche
   abstraite qui ne sera jamais exécutée, et
   destinée à être combinée avec d'autres tâches pour créer une commande
   complexe

**Exemple : **

```bash
$ ./tadmor -s 81 82 83 # Création d'une tâche en séquence qui s'éxécute à chaque minute à partir des tâches 81 82 83
84
$ ./tadmor -p -d 2 81 82 83 # Création d'une tâche en pipeline qui s'éxécute tous les mercredis à partir des tâches 81 82 83
84
$ ./tadmor -i -H 1 81 82 83 # Création d'une tâche conditionnelle qui s'éxécute tous les jours à 1h à partir des tâches 81 82 83
84
```

#### Supression de tâches : 

* `-r TASKID` : suppression de la tâche d'identifiant `TASKID`

#### Consultation des données des tâches : 

* `-l` : liste des tâches avec identifiant

* `-x TASKID` : liste datée des valeurs de retour des exécutions de la
   tâche d'identifiant `TASKID`

* `-o TASKID` : sortie standard de la dernière exécution complète de la
   tâche d'identifiant `TASKID`

* `-e TASKID` : erreur standard de la dernière exécution complète de la
   tâche d'identifiant `TASKID`

#### Arrêt d'erraid :

* `-q`

#### Modification du répertoire de communication : 

* `-P PIPES_DIR` : définition du répertoire contenant les tubes de
   communication avec le démon (par défaut `/tmp/$USER/erraid/pipes`)

**Exemple :**
```bash
$ ./tadmor -P toto #Les tubes sont maintenant dans le dossier du projet dans toto/pipes
```

### Le démon 'erraid'
```bash
./erraid
```

- Fonctionnement : Le démon erraid crée son répertoire de travail `tmp/$USER/erraid` pour faire ce que demande tadmor et y mettre les tâches à éxécuter.
- Logs : L'activité du démon (erreurs, exécutions) est consignée dans le fichier ``log situé dans son répertoire de fonctionnement (pour aider à gérer les erreurs).

### Options de la commande erraid : 

#### Exécution en avant-plan dans le terminal sans démonisation :

* `-F`

#### Définition du répertoire de stockage : 

* `-R RUN_DIR`

#### Définition du répertoire contenant les tubes de communication :

* `-P PIPES_DIR`

**Exemples d'Utilisation**

```bash
$ ./erraid # Lancement dans le répertoire par défaut
$ ./erraid -F -R /home/toto/task -P /home/toto/document/tubes # Lancement en avant-plan et définition du répertoire de travail dans /home/toto/task et création des tubes dans /home/toto/document/tubes
```

⚠️ ##Attention : 

- on peut faire mettre comme option -FR mais pas -RF et ni -RP ou -PR. Il est préférable de le faire comme dans l'exemple.
- Tuez le démon avec tadmor -q plutôt qu'avec un SIGKILL. SIGKILL ne va pas laisser le démon le temps de libérer sa mémoire. Faire ainsi peut entraîner des fuites.

Pour plus de précisions, veuillez lire le fichier [ARCHITECTURE.md](ARCHITECTURE.md).



## Auteurs
Voir le fichiers [AUTHORS.md](AUTHORS.md) pour voir la liste des membres de l'équipe.