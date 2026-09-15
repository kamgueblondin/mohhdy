# ASSIST-051 / 052 / 053 - install PC, hyperviseur, cloud (scaffold)

**Date :** 15 septembre 2026
**Statut :** packaging operateur livre ; pas un SaaS de facturation ; pas une image disque preconstruite
**Ponctuation :** ASCII usuel et accents français uniquement

Meme contrat HTTP que Docker ([assist050_docker_runtime.md](assist050_docker_runtime.md)) :
`/health`, `/embed.js`, `/admin`, `/demo`, `/demo-app`, `/browser`, `/browser/fs`,
sessions, simulateur DOM. FS sandbox : [assist060_061_browser.md](assist060_061_browser.md).
Spec : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md).
Plan : [PLAN_SUITE_IMPLEMENTATION.md](PLAN_SUITE_IMPLEMENTATION.md).

Ce n'est **pas** `make iso` ni `build/mohhdy.bin` (prototype i386 Multiboot).
Ce n'est **pas** un LLM de production. Aucun secret dans le depot.
`make ci` et `make integration-qemu` ne sont pas allonges.

## Chemins de lancement

| Chemin | Commande | Quand |
|---|---|---|
| Fumee locale (deja 050) | `make agent-smoke` | Sans Docker, Python stdlib |
| Docker | `docker run -p 8080:8080 mohhdy-agent` | Vehicule principal |
| PC natif (051) | `agent/scripts/install.sh --prefix DIR --start` | Sans Docker |
| Hyperviseur (052) | cloud-init + QEMU x86_64, ou compose dans une VM Linux | Isoler le runtime |
| Cloud heberge (053) | `MOHHDY_AGENT_MODE=hosted` + `SITE_ID` | Label d'instance, **pas** Stripe |

Variables identiques sur tous les chemins :

- `MOHHDY_AGENT_HOST` (defaut `0.0.0.0`)
- `MOHHDY_AGENT_PORT` (defaut `8080`)
- `ADMIN_TOKEN` (optionnel, execution seulement)
- `MOHHDY_AGENT_CONFIG` / `MOHHDY_AGENT_KB` / `MOHHDY_AGENT_DATA`
- `MOHHDY_AGENT_MODE` : `self_host` (defaut) ou `hosted`
- `MOHHDY_AGENT_RUNTIME` : `docker` (defaut) ou `browser` (ASSIST-061)
- `MOHHDY_AGENT_BROWSER_ENGINE` : `off` (defaut), `playwright` ou `chromium` (optionnel, hors slim)
- `MOHHDY_AGENT_SITE_ID` : identifiant d'instance / locataire (optionnel)

`GET /health` publie `deployment_mode`, `billing=none`, `quota.enforced=false`.
Le champ `quota` est un **placeholder**. Il ne debite rien. Il n'appelle
aucun reseau payant.

## ASSIST-051 : installation PC (Linux)

Python 3.9+ (3.12 recommande, comme l'image Docker). Aucun paquet pip.

Depuis la racine du depot :

```text
make agent-install-check
agent/scripts/install.sh --prefix /tmp/mohhdy-agent --start --port 18080
```

`--check` copie vers un prefixe temporaire, demarre `server.py`, verifie
`/health`, puis arrete le processus. C'est la fumee d'installateur.

Installation persistante, exemple :

```text
sudo useradd --system --home /opt/mohhdy-agent --shell /usr/sbin/nologin mohhdy
sudo agent/scripts/install.sh --prefix /opt/mohhdy-agent --systemd
sudo cp agent/packaging/mohhdy-agent.env.example /etc/mohhdy-agent.env
# Editer /etc/mohhdy-agent.env : ADMIN_TOKEN, MOHHDY_AGENT_CONFIG, MODE.
sudo install -m 644 agent/packaging/mohhdy-agent.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now mohhdy-agent
curl -fsS http://127.0.0.1:8080/health
```

Le unit systemd est un **exemple**. Adapter `User=` et les chemins.
Ne jamais cuire `ADMIN_TOKEN` dans le fichier `.service` versionne.

Premier plan, sans systemd :

```text
MOHHDY_AGENT_CONFIG=agent/config.example.json \
  ADMIN_TOKEN=change-me-at-runtime \
  python3 agent/server.py
```

Embed : meme snippet que Docker (`<script src="https://INSTANCE/embed.js">`).

### macOS (notes courtes)

`python3` de l'OS ou Homebrew. Meme `install.sh --prefix`. Pas de unit
systemd ; `launchd` n'est pas fourni. Docker Desktop reste optionnel
(chemin 050).

### Windows (notes courtes)

Python 3 depuis python.org. Depuis PowerShell, a la racine du depot :

```text
$env:MOHHDY_AGENT_HOST="127.0.0.1"
python agent\server.py
```

Pas d'installateur MSI. WSL2 peut reutiliser le script Linux.

## ASSIST-052 : hyperviseur (pas l'ISO AOS)

Deux recettes, **distinctes** du hobby kernel :

1. **Compose dans une VM Linux** : installer Docker dans Ubuntu/Debian,
   cloner ou copier le depot, puis `cd agent && docker compose up --build`.
   Meme API. Image `mohhdy-agent`, pas `mohhdy.bin`.
2. **QEMU/KVM + cloud-init** : une cloud image Linux **x86_64** (Ubuntu
   cloud, Debian cloud, etc.). Stub NoCloud :
   `agent/packaging/cloud-init/user-data` et `meta-data`.
   Commande documentee : `agent/packaging/qemu-agent.cmd`.
   Script : `agent/scripts/qemu-agent.sh --dry-run`.

Fumee automatique, sans telecharger d'image, sans boot :

```text
make agent-hypervisor-dry-run
```

Le stub cloud-init installe `python3`, prepare `/opt/mohhdy-agent` et un
fichier d'environnement **sans jeton**. L'operateur copie ensuite
`agent/` (scp, cloud-init `write_files`, ou volume). ADMIN_TOKEN uniquement
a l'execution.

Exemple QEMU une fois l'image fournie par l'operateur (hors CI) :

```text
genisoimage -output cidata.iso -volid cidata -joliet -rock \
  agent/packaging/cloud-init/user-data \
  agent/packaging/cloud-init/meta-data
CLOUD_IMAGE=/var/lib/libvirt/images/ubuntu-cloud.qcow2 \
  SEED_ISO=cidata.iso \
  agent/scripts/qemu-agent.sh --boot
```

Redirection utilisateur : hote `8080` vers invite `8080`. Verifier
`http://127.0.0.1:8080/health` puis coller `/embed.js`.

Interdit :

- `qemu-system-i386 -kernel build/mohhdy.bin`
- `make iso` GRUB du prototype comme substitut du runtime agent
- secret dans `user-data` versionne
- Packer n'est pas requis (stub cloud-init suffit)

## ASSIST-053 : self-host vs heberge (bootstrap, pas de billing)

| Mode | Qui opere | Variable | Statut livre |
|---|---|---|---|
| Self-host | Le client (Docker, PC, hyperviseur) | `MOHHDY_AGENT_MODE=self_host` | Complet pour un operateur unique |
| Heberge | Une offre qui tourne **cette** image pour le client | `MOHHDY_AGENT_MODE=hosted` | Label + locataires de config. **Pas** de paiement |

Self-host reste possible. Rien n'oblige un compte vendeur.

Locataires : les cles `sites` de `MOHHDY_AGENT_CONFIG` (deja ASSIST-030).
`GET /api/admin/status` liste `tenants[].site_id`. `instance.site_id` ou
`MOHHDY_AGENT_SITE_ID` identifie l'instance.

Quota JSON (exemple dans `agent/config.example.json`) :

```text
quota.billing = false
quota.enforced = false
quota.max_sessions / max_sites / max_messages_per_session
```

Meme si le fichier met `billing: true`, le serveur **force** `billing=none`
et `enforced=false`. Ce n'est pas Stripe, pas une API publique payante,
pas un compteur de factures clients (la facture demo MCP reste locale).

Hors perimetre (SaaS ulterieur) :

- comptes opérateurs multi-org, SSO, isolation reseau par locataire
- pretelvements, TVA, portail d'abonnement
- provision automatique d'origine HTTPS geree par un vendeur
- appels OpenAI / reseau payant depuis la CI

## Cibles Make (hors `make ci`)

```text
make agent-smoke
make agent-install-check
make agent-hypervisor-dry-run
make agent-deploy-check
make agent-docker
```

`agent-deploy-check` enchaine install-check et hypervisor-dry-run.
Aucune de ces cibles n'entre dans `make ci` ni `make integration-qemu`.
Le job GitHub `agent-http-smoke` reste parallele au build i386.

## Honnêteté

- Scaffold de packaging, pas une marketplace cloud.
- Pas d'image qcow2 stockee dans Git (poids, licence cloud image).
- Pas de Packer obligatoire.
- Chromium et LLM de production restent hors livraison.
