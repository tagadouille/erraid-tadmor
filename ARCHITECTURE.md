# Architecture du projet

## Vue d'ensemble

L'architecture du projet est conçue comme un système client-serveur local pour la planification et l'exécution de tâches.
Nous avons :
- `erraid` (démon) : Gère le cycle de vie des tâches, la planification et l'exécution de celles-ci.
- `tadmor` (client) : Interface utilisateur pour intéragir avec le démon grâce aux requêtes.

## Organisation

Le projet est structuré de telle façon à séparer les interfaces (fichiers .h) dans un répertoire `include` et l'implémentation (fichiers .c) dans un autre répertoire nommé `src`.

### Include
Ce répertoire contient tous les fichiers d'en-tête. Il définit les structures de données partagées et les contrats d'interface.
- Définition des types : Comme `timing_t`, `arguments_t` et d'autres.
- Protocoles : Définit les constantes et les structures de messages de communication.
- Et regroupe les configurations globales, tels que les chemins par défaut, les constantes en Big-Endian.

### Src
Comme le répertoire 'include/', ce répertoire est séparé en plusieurs sous répertoires qui est organisé de la même manière que 'include/' :
- **erraids/** : Qui contient toute la logique du démon (détails à préciser)
- **tadmors/** : Code source du client en ligne de commande.
- **communication/** : Gestion des pipes et sous répertoire `serialization` pour l'encodage Big-Endian.
- **tree-reader/** et **tree-writer** : Gère la création et la lecture d'arborescence des tâches.
- **types/** : Structures de données internes (Manipulation des chaînes de caractères, calcul de timing...)

## Respect du sujet (le titre à revoir)
- Appels Système
- Sérialisation : Les données numériques sont stockées en format Big-Endian pour assurer l'indépendance vis-à-vis de l'architecture matérielle.
- 
