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
```bash
./tadmor
```

Le client tadmor propose plusieurs options comme demandé dans l'énoncé donné : 
- Ajout de tâche (Création) : Permet de définir une commande et son timing
- Consultation : Permet de lister les tâches, les sorties, ou les valeurs de retour des tâches.
- Supression

### Le démon 'erraid'
```bash
./erraid
```

- Fonctionnement : Le démon erraid crée son répertoire de travail `tmp/$USER/erraid` pour faire ce que demande tadmor.
- Logs : L'activité du démon (erreurs, exécutions) est consignée dans le fichier ``log situé dans son répertoire de fonctionnement (pour aider à gérer les erreurs).

Pour plus de précisions, veuillez lire le fichier [ARCHITECTURE.md](ARCHITECTURE.md).

## Auteurs
Voir le fichiers [AUTHORS.md](AUTHORS.md) pour voir la liste des membres de l'équipe.