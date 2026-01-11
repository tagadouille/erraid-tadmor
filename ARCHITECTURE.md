# Architecture du projet

## Vue d'ensemble

L'architecture du projet est conçue comme un système client-serveur local pour la planification et l'exécution de tâches.
Nous avons :
- `erraid` (démon) : Gère le cycle de vie des tâches, la planification et l'exécution de celles-ci.
- `erraid-servant` (démon) : le fils d'erraid qui permet de répondre aux requêtes de tadmor pendant que son père gère l'éxécution des tâches
- `tadmor` (client) : Interface utilisateur pour intéragir avec le démon grâce aux requêtes.

## Organisation

Le projet est structuré de telle façon à séparer les interfaces (fichiers .h) dans un répertoire `include` et l'implémentation (fichiers .c) dans un autre répertoire nommé `src`.

### Include
Ce répertoire contient tous les fichiers d'en-tête. Il définit les structures de données partagées et les contrats d'interface.
- Définition des types : Comme `timing_t`, `arguments_t` et d'autres.
- Protocoles : Définit les constantes et les structures de messages de communication.
- Et regroupe les configurations globales, tels que les chemins par défaut, les constantes en Big-Endian.
- Il y a aussi  la documentations des différentes fonctions non statique

### Src
Comme le répertoire 'include/', ce répertoire est séparé en plusieurs sous répertoires qui est organisé de la même manière que 'include/' :
- **erraids/** : Qui contient toute la logique du démon (détails à préciser)
- **tadmors/** : Ce qui concerne tadmor
- **communication/** : Gestion des pipes de communications et sous répertoire `serialization` pour l'encodage et décodage dans les pipes
- **tree-reading/** et **tree-writer** : Gère la création et la lecture d'arborescence des tâches.
- **types/** : Structures de données internes liés aux tâches (Manipulation des chaînes de caractères, calcul de timing...)

## Explication détaillé des différents répertoires sources :

#### erraids

Il contient les fonctions qu'utilisent erraid pour mener à bien ses objectifs.

- **erraid-scanner** permet de scanner les tâches dans l'arborescence à l'aide des fonctions de tree-reader et d'ordonner l'éxécution des tâches toutes les minutes à l'aide de la fonction globale `erraid_scan_loop`
- **erraid-executor** permet de déterminer si une tâche doit être éxécuter avec `run_task_if_due` et de gérer son éxécution à l'aide de différentes fonctions mais aussi de s'occuper de l'ajout dans times-exitcodes de l'exitcode obtenu.
- **executor-sp** est un fichier de fonctions helper pour `erraid-executor` qui permet de gérer l'éxécution des tâches PL et IF.
- **erraid-log** contient la fonction `write_log_msg` qui permet d'écrire dans le fichier log les logs du démon pour savoir ce qu'il se passe à l'éxécution à des fins de debugs
- **erraid-servant** est la partie du démon qui s'occupe de répondre aux requêtes du client en lisant le pipe de requête et en répondant dans le pipe de réponse. Mais aussi de gérer la requêtes. `start_serve` permet de le lancer.
- **erraid_helper** contient des fonctions helper pour l'initialisation du démon et du répertoires de pipes et cleanup du démon.
- **erraid** Contient les gestionnaires de signaux, `daemon_init` pour l'initialisation du démon et `daemon_run` pour lancer la partie du démon qui fait le scan de l'arborescence et `erraid-servant` pour gérer les requêtes

------------------

#### tadmors

Il contient les fonctions d'interprétation et parsing des commandes de tadmor et d'envoie et réception de requêtes dans **tadmor_main** et aussi d'affichage des réponses ennvoyé par erraid dans **tadmor**.

------------------

#### tree-reading

Il contient des fonctions de lecture d'arborescence de tâches. Il y a des fichiers C pour lire les données spécifiques des tâches. Comme le timing et times-exitcodes avec **times_reader** ou les sorties avec **output_reader**.
Il y a aussi un fichie plus globale qui permet de faire tout cela à la fois en appelant les fonctions des fichiers spécifiques qui s'appelle **tree-reader**.

- `task_reader` permet de lire l'arborescence d'une tâche individuelle selon Action_type qui est le type de l'action que l'on veut faire. C'est une fonction générale
- `all_task_listing` permet de lister toutes les tâches de tasks dans un tableau de tâche pour l'éxécution.

Le reste des fonctions sont des helpers pour mené à bien ce but là.

------------------

#### tree-writer

Il contient des fonctions de modifications d'arborescence de tâches. Que cela soit de la suppression avec **task_annihilator** ou de la création de tâche avec **task_creator**

- **task_annihilator** permet de supprimmer une arborescence entière, il est utiliser pour supprimer des tâches
- **task_creator** permet de créer des arborescences de tâches SI. Il y a des fonctions de créations et de remplissage des fichiers timing, argv, type et etc. Et une fonction de création de tâche.
- **task_combinator** permet de combiner des tâches. Il utilise les fichiers mentionner précédemment pour cela dans une seule fonction qui est `combine_and_destroy_tasks`. Elle créer une nouvelle tâche et dans cmd met les arborescences des tâches voulues en omettant le fichier timing et supprime l'arborescence ensuite.

------------------
#### types

Il contient ce qui concerne les différents types de données concernant les tâches. Il y a des fonctions de copie, de free et de création de structure.

- **arguments** pour les arguments de commandes
- **command** pour les commandes
- **my-string** pour la structure string
- **task** pour les tâches
- **time-exitcode** pour times-exitcodes
- **timing** pour le timing

------------------
#### communication

Ce dossier contient ce qui concerne la communication entre le client et erraid. 
Il y a les structures de communications qui y sont définies avec bien sûr des méthodes de créations
et de libération de mémoire.

- **answer** pour les structures de réponses
- **request** pour les structures de requête

Il y a aussi des fonctions qui gèrent la communication entre processus

- **pipes** pour la gestion des tubes de communication. Il y a des fonctions d'ouvertures de tubes et des fonctions d'initialisation des tubes et de récupération du chemin des tubes. 
- **communication** contient les fonctions principales de communication pour écrire dans les tubes et lire depuis les tubes de requêtes et de réponses pour le démon et pour le client.

Pour que les processus déterminent où se trouvent les tubes un fichier pipe_path.te est créer et remplis dans le répertoire du projet. Les processus le lisent et le modifie au besoin.

- **request-handle** est un fichier permettant à `erraid-servant` de répondre aux requêtes. Il y a diverses fonctions qui permettent de faire ce que la requête qui lui a été envoyé demande. Les deux fonctions principales pour cela sont : `complex_request_handle` pour les requêtes complexes et `simple_request_handle` pour les requêtes simples.

------------------
#### serialization

C'est un sous-dossier contenu dans communication qui contient des fichiers qui permettent la sérialisation des données et l'envoie des données sérialisés dans les tubes mais aussi leur récupération.

- **decode-request** contient toutes les fonctions permettant de décoder les requêtes.
- **decode-response** contient toutes les fonctions permettant de décoder les réponses.
- **encode-request** contient toutes les fonctions permettant d'encoder les requêtes.
- **encode-response** contient toutes les fonctions permettant d'encoder les réponses.
- **en_decode_struct** contient toutes les fonctions permettant le décodage et l'encodage des structures de données dans les tubes.
- **serialization** contient toutes les fonctions de sérialisation de données (les uint8/16/32/64 et etc) et d'envoie de ces données dans les tubes avec `write_full` et de réception avec `read_full`.