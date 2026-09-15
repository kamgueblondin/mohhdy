# ASSIST-050 - image Docker du runtime agent (scaffold)

**Date :** 15 septembre 2026
**Statut :** scaffold livré, **pas** le produit Agent Support complet
**Ponctuation :** ASCII usuel et accents français uniquement

Ce document décrit comment construire, lancer et vérifier le conteneur
HTTP du track Agent Support. Il n'héberge pas le noyau Multiboot i386.
Il n'allonge pas `make integration-qemu`. Il n'entre pas dans `make ci`.

Spec produit : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md)
(ASSIST-050). Plan : [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md).

## Ce qui est livré (scaffold)

Arborescence parallèle `agent/` (Python 3, bibliothèque standard uniquement) :

| Route | Réponse |
|---|---|
| `GET /health` | 200 JSON `{"status":"ok","service":"mohhdy-agent"}` |
| `GET /admin` | Coquille HTML admin, liste de sessions **vide** |
| `GET /embed.js` | Snippet public : bulle de chat (stub), **sans secret** |
| `GET /demo` | Page statique hote qui charge `embed.js` |
| `GET /` | Index des routes, bandeau scaffold |

Honnêteté produit :

- Le chat IA ne répond pas.
- Aucune session visiteur n'est ouverte (ASSIST-011 reste ouvert).
- L'admin n'authentifie personne et n'affiche aucune file (ASSIST-040 / 041).
- Aucun geste navigateur ni outil MCP (ASSIST-020..022).
- Le noyau Multiboot **n'est pas** booté dans ce conteneur.

## Prérequis

- Python 3.12+ pour la fumée locale (`make agent-smoke`) : déjà le Python hôte du prototype.
- Docker (BuildKit optionnel) **seulement** pour l'image. Absent de la CI AOS.

Aucun paquet pip. Aucun `.env` n'est lu au démarrage.

## Construire et lancer (`docker run`)

Depuis la racine du dépôt :

```text
docker build -t mohhdy-agent ./agent
docker run --rm -p 8080:8080 mohhdy-agent
```

Cible Make equivalente (ne touche pas QEMU) :

```text
make agent-docker
docker run --rm -p 8080:8080 mohhdy-agent
```

Compose local, depuis `agent/` :

```text
cd agent
docker compose up --build
```

Le service écoute `0.0.0.0:8080`. Surcharge possible, sans secret :

- `MOHHDY_AGENT_HOST` (défaut `0.0.0.0`)
- `MOHHDY_AGENT_PORT` (défaut `8080`)

Ne pas passer de jeton OpenAI, de `.env` ou de `env_file`. L'image est
construite utilisateur non-root `mohhdy` (uid 10001). `.dockerignore`
exclut `.env`, clés et tests.

## Vérifier

Fumée stdlib, **sans Docker**, hors gate AOS :

```text
make agent-smoke
```

Cette cible démarre le serveur sur un port éphémère et contrôle health,
admin, `embed.js` (absence de marqueurs de secret) et `/demo`. Elle n'est
**pas** appelée par `make ci` ni par `make integration-qemu`.

Fumée contre une origine déjà lancée (conteneur ou `python3 agent/server.py`) :

```text
python3 agent/server.py
BASE_URL=http://127.0.0.1:8080 agent/scripts/smoke.sh
```

Controles manuels :

```text
curl -fsS http://127.0.0.1:8080/health
curl -fsS -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/admin
curl -fsS -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/embed.js
curl -fsS -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/demo
```

Ouvrir `http://127.0.0.1:8080/demo` : la bulle en bas à droite s'affiche.
Le champ de saisie est désactivé (stub).

## Ce que l'image ne contient pas

- Secret, `.env`, clé API, certificat privé
- Binaire noyau `build/mohhdy.bin`, initrd, ISO GRUB
- Dépendance pip, Node, navigateur outillé
- Modèle GPT-2 / GGUF

## Relation au prototype AOS

Le hobby OS i386 reste la couche vérifiée ([ETAT_REEL.md](ETAT_REEL.md)).
Ce scaffold ne remplace pas QEMU, ne s'ajoute pas aux sept contrats
d'intégration, et ne doit pas faire grandir le job `integration-qemu`.
Un job CI optionnel `agent-http-smoke` (Python stdlib, sans Docker) peut
tourner en parallèle ; il ne `needs` pas le build i386.
