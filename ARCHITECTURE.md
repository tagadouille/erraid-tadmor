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

## Respect du sujet (le titre à revoir)
- Appels Système
- Sérialisation : Les données numériques sont stockées en format Big-Endian pour assurer l'indépendance vis-à-vis de l'architecture matérielle.
- 

## Explication détaillé des différents répertoires sources :

#### erraids

Il contient les fonctions qu'utilisent erraid pour mener à bien ses objectifs.

- **erraid-scanner** permet de scanner les tâches dans l'arborescence à l'aide des fonctions de tree-reader et d'ordonner l'éxécution des tâches toutes les minutes.
- **erraid-executor** permet de déterminer si une tâche doit être éxécuter avec _run_task_if_due_ et de gérer son éxécution à l'aide de différentes fonctions mais aussi de s'occuper de l'ajout dans times-exitcodes de l'exitcode obtenu.
- **executor-sp** est un fichier de fonctions helper pour _erraid-executor_ qui permet de gérer l'éxécution des tâches PL et IF.
- **erraid-log** contient la fonction _write_log_msg_ qui permet d'écrire dans le fichier log les logs du démon pour savoir ce qu'il se passe à l'éxécution à des fins de debugs
- **erraid-servant** est la partie du démon qui s'occupe de répondre aux requêtes du client en lisant le pipe de requête et en répondant dans le pipe de réponse. Mais aussi de gérer la requêtes. _start_serve_ permet de le lancer.
- **erraid_helper**
- **erraid**

#### tadmors

Il contient les fonctions d'interprétation et parsing des commandes de tadmor et d'envoie et réception de requêtes dans **tadmor_main** et aussi d'affichage des réponses ennvoyé par erraid dans **tadmor**.
#### tree-reading

Il contient des fonctions de lecture d'arborescence de tâches. Il y a des fichiers C pour lire les données spécifiques des tâches. Comme le timing et times-exitcodes avec **times_reader** ou les sorties avec **output_reader**.
Il y a aussi un fichie plus globale qui permet de faire tout cela à la fois en appelant les fonctions des fichiers spécifiques qui s'appelle **tree-reader**.

- _task_reader_ permet de lire l'arborescence d'une tâche individuelle selon Action_type qui est le type de l'action que l'on veut faire. C'est une fonction générale
- _all_task_listing_ permet de lister toutes les tâches de tasks dans un tableau de tâche pour l'éxécution.

Le reste des fonctions sont des helpers pour mené à bien ce but là.
#### tree-writer

Il contient des fonctions de modifications d'arborescence de tâches. Que cela soit de la suppression avec **task_annihilator** ou de la création de tâche avec **task_creator**

- **task_annihilator** permet de supprimmer une arborescence entière, il est utiliser pour supprimer des tâches
- **task_creator** permet de créer des arborescences de tâches SI. Il y a des fonctions de créations et de remplissage des fichiers timing, argv, type et etc. Et une fonction de création de tâche.
- **task_combinator** permet de combiner des tâches. Il utilise les fichiers mentionner précédemment pour cela dans une seule fonction qui est combine_and_destroy_tasks. Elle créer une nouvelle tâche et dans cmd met les arborescences des tâches voulues en omettant le fichier timing et supprime l'arborescence ensuite.

#### types
#### communication
#### serialization
