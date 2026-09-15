# Capacites OS de l'instance Mohhdy (backlog ASSIST-xxx)

**Date :** 15 septembre 2026
**Statut :** backlog de **capacites du SE**, pas un produit distinct. ASSIST-050/010/011/012/013/020/021/022/030/031/040/041 = scaffold userspace HTTP livre (stub local, sessions, KB, droits, escalade, handoff, admin a jeton, simulateur DOM, MCP demo, facture mock, refus d'origine). ASSIST-051/052/053 = packaging operateur (install PC, recette hyperviseur, scaffold cloud non-billing). ASSIST-060/061 = vue `/browser` et FS sandbox `/api/browser/fs` (pas US-031). Playwright / Chromium = **profil optionnel** operateur, pas US-031. LLM de production, navigateur-OS complet et SaaS de paiement **pas** livres
**IDs :** `ASSIST-xxx` (ne collident ni avec `AOS-xxx` ni avec `US-xxx`)
**Ponctuation :** ASCII usuel et accents français uniquement

Ce document decrit les **devoirs du SE Mohhdy** : tenir un support client sur le web, agir dans le **navigateur-OS du SE**, rester autonome, et ceder la main a un humain. Ce n'est **pas** une piste parallele, pas un widget SaaS a cote du noyau. `agent/` est le **scaffold userspace actuel** de l'instance jusqu'a ce que davantage vive dans le guest : widget `embed.js`, sessions visiteur isolees, base de connaissance locale optionnelle, masque de droits, escalade, handoff dans la meme session, console `/admin` (jeton `ADMIN_TOKEN` optionnel), simulateur de gestes sur `/demo-app`, outil MCP demo, facture mock, vue `/browser` (miroir du simulateur + controle Playwright optionnel) et FS sandbox `/browser/fs`. Les reponses sont un **stub local** (echo ou extraits de KB), pas un LLM de production. Playwright n'est **pas** une dependance du slim. **Pas** US-031.

En cas de contradiction sur ce qui **tourne aujourd'hui**, [../docs/ETAT_REEL.md](../docs/ETAT_REEL.md) et [mohhdy_us.md](mohhdy_us.md) priment.

Plan d'ordre : [../docs/PLAN_SUITE_IMPLEMENTATION.md](../docs/PLAN_SUITE_IMPLEMENTATION.md). Index : [README.md](README.md).

## Positionnement (un produit, deux niveaux de maturite)

Mohhdy est **un seul** SE agentique autonome. Deux niveaux de runtime, pas trois produits :

| Niveau | Ce qu'il dit | Ce qu'il n'est pas |
|---|---|---|
| Prototype guest | Chemin noyau i386 Multiboot sous QEMU (AOS-001 a AOS-026 verifies) | Un runtime web, un widget JS, une image Docker, le SE autonome complet |
| Specs historiques | Phases 1-8 (Foundation, AI Core, Web Runtime, etc.) | Un second produit, un backlog de build du guest |
| **Instance OS autonome** | Capacites du SE : support sur le web, navigateur-OS, admin, outils, Docker / PC / hyperviseur / machine vierge | Un produit "Agent Support" a cote, une fonction deja livree dans le noyau i386 |

Le widget JS, la console admin, les actes navigateur et Docker exigent un **userspace plus large** qu'un noyau Multiboot i386 nu. Ce backlog ne pretend pas les faire tourner dans le shell Ring 3 actuel. Docker **boot l'instance** comme sur une machine vierge.

Vocabulaire **reutilise**, sans nouveaux IDs vision :

- **Capacites / droits** : meme idee que Foundation (grant, revoke, scope, moindre privilege). Les capabilities AOS actuelles sont locales, volatiles et liees a un PID Ring 3. L'instance a besoin d'un modele equivalent **par site, par session et par outil**, pas d'un renumerotage de US-001 / US-003.
- **Assistant / IA** : voisinage de US-021 (assistant integre) et US-028 (intelligence conversationnelle). Le guest n'offre que `ai <texte>` synchrone et borne (AOS-010). L'instance porte l'assistant pour le support. Ce n'est **pas** TensorFlow Lite (US-016).
- **Navigateur-OS du SE** : **coeur produit**, voisinage de la phase 3 (US-031 a US-033 dans [mohhdy_us_phase3_web_runtime.md](mohhdy_us_phase3_web_runtime.md)). ASSIST-060/061 en sont le bootstrap. US-031 **n'est pas** livre.
- **Deploiement** : voisinage de US-015 et de la phase 6 / phase 8. Docker / PC / hyperviseur = **deploiement du SE**, pas un packaging d'un sidecar.
- **Actions dans l'appli cliente** : voisinage de US-034 (connecteurs entreprise) et d'outils MCP / allowlist, pas un ERP livre.

Dependances honnetes vers le guest :

- Les priorites AOS proches (budget CI, ACL prefixe, latence GGUF, migration stockage) **restent** les tranches 0-4. Ce backlog ne les remplace pas.
- OpenAI public reste **sous condition** (accord explicite, secret hors image, hors CI).
- Docker peut **commencer avant** un microkernel US-001 complet : l'instance n'a pas a attendre que ATA / FAT quittent le noyau i386.

## Vision

Mohhdy n'est plus seulement un prototype pedagogique i386. C'est le **SE agentique autonome** : une instance que l'operateur deploie (Docker, PC, hyperviseur, machine vierge), que le visiteur d'un site rencontre via un embed JavaScript (UX proche de tawk.to), et qui **agit** au lieu de seulement discuter.

L'agent :

1. Discute avec le visiteur et explique la plateforme hôte.
2. Exécute uniquement les actions autorisées sur le site (clics, souris, outils / MCP du site).
3. Respecte les droits et capacités de la session.
4. Escalade vers un humain seulement si le visiteur le demande ou si la politique l'exige.
5. Laisse un humain **continuer** la même conversation depuis une console d'administration.

Si, plus tard, MOHHDY a un corps physique, les mêmes droits pourront gouverner des actes physiques. C'est une piste **future** (ASSIST-090), pas une livraison proche.

## Personas

| Persona | Besoin | Hors périmètre proche |
|---|---|---|
| Visiteur d'un site | Obtenir une réponse, une explication, une action autorisée, sans installer quoi que ce soit | Administrer l'instance, voir d'autres sessions |
| Opérateur de site (marchand) | Coller un script d'embed, déclarer les outils autorisés, relire les échanges | Recompiler le noyau i386 |
| Humain de support | Voir les sessions, reprendre le fil, clore ou transférer | Contourner la politique de droits |
| Opérateur d'instance (self-host) | Lancer Docker / PC / hyperviseur, exposer l'origine HTTPS de l'embed | Dépendance à un cloud vendeur |
| Abonné cloud | La même chose sans opérer le runtime | Propriété exclusive du modèle d'hébergement |
| Opérateur physique (futur) | Étendre les capacités à un corps | Livraison proche |

## Non-objectifs (explicites)

- Ne pas déclarer livrés le produit complet : LLM de production, Chromium réel, ERP, marketplace SaaS payante. Le runtime HTTP (`agent/`) n'est pas ce produit. Les gestes sont un simulateur DOM local. L'install PC / la recette hyperviseur / le mode `hosted` sont un **scaffold** de packaging, pas un abonnement facturé.
- Ne pas deplacer le backlog AOS (CI 25 min, ACL prefixe, GGUF, stockage hors noyau) hors du guest.
- Ne pas heberger l'embed public **dans** le noyau Multiboot i386 actuel.
- Ne pas ouvrir TensorFlow Lite, NLU federé, P2P, economie de points, ou US-001 "d'un coup".
- Ne pas appeler un hote OpenAI public depuis la CI, ni placer un secret dans le JS d'embed.
- Ne pas livrer un corps physique, ni un robot, dans les epiques proches.
- Ne pas inventer une troisieme numerotation `US-xxx` ni recycler `AOS-xxx` pour le web.
- Ne pas vendre `agent/` comme un produit "Agent Support" distinct.

## Matrice de déploiement

| Cible | Rôle | Runtime supposé | Statut |
|---|---|---|---|
| Prototype guest i386 | Chemin pedagogique Multiboot, boot QEMU / ISO | Multiboot, shell Ring 3, GPT-2 / GGUF local, NE2000 local | **Verifie** (ETAT_REEL). N'heberge pas le widget |
| Conteneur Docker | **Boot de l'instance Mohhdy** + origine de l'embed + admin | Userspace avec HTTP(S), file de sessions, simulateur DOM | Scaffold `agent/` : embed, sessions, KB locale, droits, escalade, handoff, admin a jeton, `/demo-app`, `/browser`, FS sandbox, MCP demo. Pas de LLM de production ni de Chromium de session. Pas US-031 |
| Installation PC | Meme instance, package natif | Identique a Docker sur le fond, installateur en plus | ASSIST-051 : `agent/scripts/install.sh`, unit systemd d'exemple |
| Hyperviseur | Image VM (QEMU/KVM, autre) de l'instance, pas du seul hobby kernel | Identique a Docker, disque / reseau de VM | ASSIST-052 : cloud-init + QEMU x86_64 documentes ; pas `make iso` |
| Abonnement cloud heberge | Instance operee pour le client, meme API d'embed | Multi-tenant ou instance dediee, facturation | ASSIST-053 scaffold : `MOHHDY_AGENT_MODE`, `SITE_ID`, quotas placeholder. **Pas** de paiement |

L'utilisateur choisit : **son** Docker / cloud, ou l'offre hebergee. Les deux exposent le meme contrat d'embed et de droits. Le guest i386 reste le chemin noyau pedagogique mesure : il peut, plus tard, dialoguer avec la meme instance, mais ce n'est pas le chemin pour coller un script sur un site marchand.

## Modèle de droits et de sécurité

Le prototype Foundation accorde deja des droits backend VFS (`read` / `mutate` / `full`), un masque de sources, un prefixe interne, une revocation et un inventaire borne. L'instance Mohhdy **reprend cette discipline**, a une autre echelle.

### Acteurs d'une session

| Acteur | Peut | Ne peut pas |
|---|---|---|
| Visiteur | Parler, demander une action, demander un humain | Élargir l'allowlist, lire d'autres sessions, obtenir un secret |
| Agent MOHHDY | Lire le contexte autorisé, proposer, exécuter un outil **déjà** accordé | Inventer un droit, contourner l'escalade, persister un secret dans le widget |
| Humain de support | Reprendre la session, voir l'historique de **cette** session | Agir hors politique, impersonner un autre site |
| Opérateur de site | Déclarer l'allowlist, révoquer, activer/désactiver l'embed | Lire les secrets d'une autre instance |

### Capacités de session (vocabulaire)

Une session porte un ensemble **borné** :

- `chat.reply` : répondre au visiteur
- `site.explain` : expliquer la plateforme hôte à partir d'une base autorisée
- `dom.click`, `dom.type`, `pointer.move` : gestes navigateur, seulement sur l'origine déclarée
- `mcp.<outil>` : un outil site / MCP nominatif (ex. `mcp.invoice.create`)
- `session.escalate` : passer à un humain
- `admin.observe` / `admin.takeover` : côté console, pas côté widget

Principes, alignés sur Foundation :

1. **Moindre privilège** : l'agent n'obtient que les outils listés par l'opérateur pour ce site.
2. **Révocation** : retirer un outil arrête les actes suivants, sans rejouer une action incertaine.
3. **Pas de fuite de borne interne** : le widget ne reçoit ni secret d'API, ni jeton d'admin, ni préfixe interne d'ACL.
4. **Corrélation** : chaque acte est rattaché à un `session_id` et un `request_id` (même idée que l'IPC VFS).
5. **Escalade** : obligatoire si le visiteur la demande, ou si l'acte sort de l'allowlist, ou si la politique métier l'exige (paiement non autorisé, donnée personnelle sensible, etc.).

RGPD et journal : voisinage de US-048 (spec). Une session de support est une donnée personnelle. Conservation bornée, accès admin authentifié, pas de log dans le JS public.

## Croquis d'embed JavaScript

UX visée : bulle de chat en coin de page, comme tawk.to, **sans** prétendre à une parité de marque. Le script public ne contient aucun secret.

```html
<script
  src="https://INSTANCE/embed.js"
  data-mohhdy-site="site_public_id"
  async>
</script>
```

Comportement (ASSIST-010 livré, gestes = simulateur ASSIST-020) :

1. Le script dessine un lanceur, puis un panneau de conversation.
2. Il ouvre une session visiteur vers l'origine de l'instance.
3. Les messages transitent en HTTP(S) vers `/api/sessions`. Les actes (clics, MCP) passent par `POST /api/sessions/{id}/tools` **si** le droit et l'origine sont accordés (simulateur DOM, pas Chromium).
4. Les réponses sont un stub local (echo ou KB), pas un LLM de production.
5. Un bouton "Parler a un humain" demande l'escalade (ASSIST-031). Après takeover, les messages humains apparaissent et l'agent ne répond plus tout seul.

Scripts d'intégration (ASSIST-013) : snippet, CSP et refus d'origine document dans [../docs/assist010_sessions_admin.md](../docs/assist010_sessions_admin.md) et [../docs/assist013_origine_embed.md](../docs/assist013_origine_embed.md).

## Console admin et handoff

La console n'est pas le shell VGA. C'est une UI web de l'instance.

Livré (ASSIST-040 lecture + ASSIST-041 écriture) : liste des sessions, filtre file humain, détail du fil, jeton `ADMIN_TOKEN` optionnel, takeover et réponse humaine dans la même session.

Encore spec : comptes opérateurs, auth par site. Gestes et refus d'origine document : ASSIST-020 (simulateur) et ASSIST-013 (embed).

- Reprise : l'humain écrit **dans la même session** ; l'agent se tait ou passe en observateur selon la politique.
- Interdit : fusionner des sessions de sites distincts, exporter un secret, agir sans `admin.takeover`.

## Agent d'actions navigateur

Objectif : l'agent ne se contente pas de répondre. Sur une origine autorisée, il peut cliquer, déplacer le pointeur, remplir un champ, appeler un outil MCP du site.

Exemple d'acte de session (ASSIST-022) : **créer une facture** dans l'application déjà utilisée par le client, avec la même session, si `mcp.invoice.create` (ou l'équivalent déclaré) est accordé. Sinon : expliquer le refus et proposer l'escalade.

Limites :

- Pas d'acte hors origine / hors allowlist.
- Pas d'acte irréversible (paiement, suppression massive) sans politique explicite ou humain.
- L'automatisation navigateur vit dans l'instance Docker / PC / hyperviseur, pas dans le guest i386.
- Tranche actuelle : **simulateur DOM** local (`/demo-app`), pas un Chromium. Détail : [../docs/assist020_gestes_mcp.md](../docs/assist020_gestes_mcp.md).

## Accès navigateur une fois déployé

Après déploiement de l'instance :

- L'opérateur ouvre l'admin et, s'il le souhaite, un navigateur de l'instance (ASSIST-060 : `/browser`, miroir du simulateur `/demo-app`).
- Si Mohhdy est lance "directement" comme runtime navigateur (`MOHHDY_AGENT_RUNTIME=browser`), il expose **son** systeme de fichiers navigateur sandbox (ASSIST-061 : `/api/browser/fs`). C'est le vocabulaire "le navigateur-OS du SE", **sans** declarer US-031 livre. [ETAT_REEL.md](../docs/ETAT_REEL.md) n'est pas mis a jour (mesure guest seulement).
- Les scripts d'attache (ASSIST-013) relient ce runtime à un site tiers pour le support.

## Modèle d'abonnement

Deux modes commerciaux, un contrat technique :

| Mode | Qui opère le runtime | Qui colle l'embed |
|---|---|---|
| Self-host | Le client (Docker, PC, hyperviseur) | Le client |
| Cloud hébergé | L'offre MOHHDY (abonnement) | Le client, vers l'origine hébergée |

L'abonnement ne remplace pas le self-host. Il ne rend pas le guest i386 "SaaS". La facturation, le multi-tenant et le quota sont ASSIST-053. Ils dependent d'une instance Docker deja amorcable, pas du microkernel.

## Corps physique (piste future)

ASSIST-090 : si MOHHDY dispose d'un corps, les actes physiques (déplacer un objet, tendre un document) réutilisent le même modèle de capacités et d'escalade. Aucune dépendance proche. Aucun matériel, aucun firmware, aucun planning de livraison dans les épiques 010-061.

## Épiques (user stories)

Convention : **En tant que** / **je veux** / **afin de**. Critère de sortie = observable. Statut implicite : **spec**.

### ASSIST-000 - Intent agentique (cadrage)

**En tant que** proprietaire produit, **je veux** que Mohhdy soit specifie comme SE agentique autonome et pas seulement comme hobby i386, **afin de** prioriser les capacites OS sans falsifier l'etat guest.

**Critere.** Ce fichier, [README.md](README.md) et [../docs/PLAN_SUITE_IMPLEMENTATION.md](../docs/PLAN_SUITE_IMPLEMENTATION.md) presentent un produit unique et deux niveaux de maturite. Aucune phrase ne dit que l'embed tourne dans QEMU i386. Aucune phrase ne vend un produit "Agent Support" distinct.

### ASSIST-010 - Widget d'embed JavaScript

**En tant que** opérateur de site, **je veux** coller un script JS qui affiche une bulle de chat, **afin que** les visiteurs parlent à MOHHDY sans quitter la page.

**Dépendances.** ASSIST-050 (origine HTTP de l'instance) ou ASSIST-053 (origine cloud). Pas AOS-001.

**Critère.** Un site statique de démo charge `embed.js`, ouvre une session, échange un message. Pas de secret dans le script.

**Progrès.** `agent/static/embed.js` ouvre la bulle, `POST /api/sessions`, envoie et sonde les messages. `/demo` charge l'embed. Réponses = echo stub (`llm=stub_echo`), pas un LLM de production. Guide : [../docs/assist010_sessions_admin.md](../docs/assist010_sessions_admin.md).

### ASSIST-011 - Session visiteur isolée

**En tant que** visiteur, **je veux** une session à moi, **afin que** mes messages ne fuient pas vers un autre visiteur ou un autre site.

**Dépendances.** ASSIST-010.

**Critère.** Deux navigateurs, deux `session_id`. L'admin d'un site ne liste pas l'autre site.

**Progrès.** `POST /api/sessions` rend un UUID distinct. Messages bornés par `session_id` (couvert par `make agent-smoke`). Filtre admin `?site_id=`. Pas encore d'auth par site : un jeton d'instance voit toute l'instance.

### ASSIST-012 - Expliquer la plateforme hôte

**En tant que** visiteur, **je veux** que l'agent explique le site (parcours, offre, limites), **afin de** m'orienter avant d'appeler un humain.

**Dépendances.** ASSIST-010, base de connaissance fournie par l'opérateur (hors noyau i386).

**Critère.** Une question "comment ça marche ?" reçoit une réponse ancrée dans la base autorisée, ou un refus honnête si la base est vide.

**Progrès.** `MOHHDY_AGENT_CONFIG` / `MOHHDY_AGENT_KB` : JSON ou fichier texte monté. Réponse `stub_kb` ancrée, ou `stub_refusal` si la base est vide. Pas un LLM de production. Guide : [../docs/assist012_droits_handoff.md](../docs/assist012_droits_handoff.md).

### ASSIST-013 - Scripts d'attache à un site tiers

**En tant que** opérateur, **je veux** un snippet et une checklist CSP / origine, **afin d'**attacher MOHHDY à n'importe quel site que je contrôle.

**Dépendances.** ASSIST-010.

**Critère.** Documentation plus snippet. Refus explicite si l'origine du document ne matche pas le site déclaré.

**Progrès.** Snippet, notes CSP et refus automatique d'origine document vs site déclaré : `403 origin_denied` + `request_id` sur create / messages / outils. Allowlist `allowed_origins` (global ou par site) dans `MOHHDY_AGENT_CONFIG`. `/demo` et `self` restent acceptés. Guide : [../docs/assist013_origine_embed.md](../docs/assist013_origine_embed.md). Ce n'est pas une auth par comptes, pas Chromium.

### ASSIST-020 - Gestes navigateur autorisés

**En tant que** visiteur, **je veux** que l'agent clique ou saisisse **dans le périmètre autorisé**, **afin d'**avancer une tâche sans que je connaisse l'UI.

**Dépendances.** ASSIST-030, ASSIST-050 (navigateur outillé du runtime). **N'entre pas** dans le guest AOS.

**Critère.** Un scénario de démo (ouvrir un menu, remplir un champ) réussit sur l'origine allowlistée et échoue hors origine, avec journal `request_id`.

**Progrès.** Simulateur DOM in-process (`harness=dom_simulator`), page `/demo-app`, allowlist d'origines (`self` ou URL). `dom.click` / `dom.type` / `pointer.move` mutent `GET /api/demo-app/state`. Origine étrangère : 403 `origin_denied` + `request_id` au journal. Playwright n'est **pas** le harness de session (profil optionnel operateur seulement). Guide : [../docs/assist020_gestes_mcp.md](../docs/assist020_gestes_mcp.md).

### ASSIST-021 - Outils MCP / outils du site

**En tant que** opérateur, **je veux** déclarer les outils du site (MCP ou équivalent) que l'agent a le droit d'appeler, **afin de** brancher MOHHDY sur mon appli sans lui donner la clef de tout.

**Dépendances.** ASSIST-030. Voisinage US-034, sans livrer un connecteur ERP générique.

**Critère.** Allowlist nominative. Un outil absent est refusé. La révocation empêche l'appel suivant.

**Progrès.** Clé `tools` du JSON config. `mcp.invoice.create` déclaré. `mcp.not_registered` : 403 `tool_undeclared`. Révocation : l'appel suivant est 403.

### ASSIST-022 - Acte métier en session (exemple : facture)

**En tant que** visiteur autorisé, **je veux** que l'agent crée une facture **dans l'application déjà en place**, **afin de** terminer l'acte sans changer d'outil ni de session.

**Dépendances.** ASSIST-021, droit `mcp.invoice.create` (ou nom déclaré). Humain si la politique l'exige.

**Critère.** Une facture de démo apparaît côté appli hôte quand le droit est là ; sinon message de refus + offre d'escalade. Même `session_id`.

**Progrès.** `mcp.invoice.create` écrit dans le mock `/demo-app` (mémoire ou `MOHHDY_AGENT_DATA/invoices.json`). Même `session_id`. Sans droit : 403 + escalade, aucune facture.

### ASSIST-030 - Droits par site et par session

**En tant que** opérateur, **je veux** accorder, limiter et révoquer des capacités par site et par session, **afin que** l'agent n'agisse que dans le périmètre voulu.

**Dépendances.** Vocabulaire Foundation (grant / revoke / scope). **Pas** besoin que US-001 soit terminé. Identité vérifiée Foundation reste un plus, pas un préalable Docker.

**Critère.** Masque de droits visible côté admin. Diagnostic public du widget sans secret ni borne interne. Preuves négatives (outil retiré, origine étrangère).

**Progrès.** Allowlist par site et par session (`grant` / `revoke` / `set`). Gestes et MCP demo exécutés s'ils sont accordés (simulateur). Widget : capacités publiques seulement (pas `admin.*`, pas `acl.`). Outil révoqué : 403, pas d'acte. Origine étrangère : 403 `origin_denied`. Le refus d'origine **document embed** vs site déclaré est livré (ASSIST-013).

### ASSIST-031 - Escalade humaine

**En tant que** visiteur, **je veux** un humain quand je le demande, ou quand la politique l'impose, **afin de** ne pas rester bloqué face à un agent.

**Dépendances.** ASSIST-011, ASSIST-040.

**Critère.** Demande visiteur : file admin. Tentative d'acte hors politique : file admin, sans exécution. L'agent n'invente pas un droit pour "aider quand même".

**Progrès.** `POST /api/sessions/{id}/escalate`, statut `waiting_human`, bouton widget. Outil hors allowlist : 403 + file, sans exécution. `session.escalate` révoqué : 403 visiteur.

### ASSIST-040 - Console d'administration des sessions

**En tant que** humain de support, **je veux** lister et relire les échanges, **afin de** contrôler ce que l'agent a dit et fait.

**Dépendances.** ASSIST-050 ou ASSIST-053, ASSIST-011.

**Critère.** Liste, filtre ouvert / escalade / clos, détail d'une session, actes et refus. Authentification opérateur.

**Progrès.** `/admin` liste, filtre file / ouvertes / prises de main, et relit les fils. Si `ADMIN_TOKEN` est défini à l'exécution, `/api/admin/*` exige `Authorization: Bearer` (jamais cuit dans l'image). Sans variable : mode stub ouvert, bandeau d'avertissement. Pas de comptes. Handoff : ASSIST-041.

### ASSIST-041 - Handoff : l'humain continue

**En tant que** humain de support, **je veux** écrire dans la **même** conversation, **afin que** le visiteur n'ait pas à se répéter.

**Dépendances.** ASSIST-040, ASSIST-031.

**Critère.** Après takeover, les messages humains apparaissent dans le widget. L'agent ne répond plus (ou seulement en observateur, selon politique). Journal de qui parle.

**Progrès.** `POST /api/admin/sessions/{id}/takeover` puis `.../messages`. Même `session_id`. Widget : messages `human`, plus de réponse agent automatique. Journal `speaker` = visitor / agent / human / system.

### ASSIST-050 - Image Docker de l'instance Mohhdy

**En tant que** operateur d'instance, **je veux** un conteneur qui sert l'embed, l'admin et le SE, **afin de** deployer Mohhdy sur une machine vierge sans attendre un microkernel i386.

**Dependances.** Aucune tranche AOS 0-4 bloquante. **Peut commencer en parallele des gardes guest.** Ne pas alourdir `make integration-qemu`.

**Critere.** `docker run` documente : sante HTTP, page admin, origine d'embed. Pas de secret dans l'image. Hors cible : faire booter le noyau Multiboot **dans** ce conteneur comme substitut du widget.

**Progrès.** Arborescence `agent/` : Python 3 stdlib, `Dockerfile` utilisateur non-root, `docker-compose.yml`, `make agent-smoke` (hors `make ci` / hors QEMU). Sert embed, sessions, KB, droits, escalade, handoff, admin, `/demo-app`, MCP demo. Guide image : [../docs/assist050_docker_runtime.md](../docs/assist050_docker_runtime.md). Gestes : [../docs/assist020_gestes_mcp.md](../docs/assist020_gestes_mcp.md). **Non livré dans l'image :** secret, LLM de production, Chromium, noyau i386.

### ASSIST-051 - Installation PC

**En tant que** opérateur, **je veux** installer le même runtime sur un PC, **afin de** m'en servir sans Docker si je le préfère.

**Dépendances.** ASSIST-050 (même contrat). Voisinage phase 6 / US-015.

**Critère.** Paquet ou installateur documenté, même API d'embed que Docker.

**Progrès.** Script `agent/scripts/install.sh` (Linux, Python stdlib, mêmes variables que Docker). Unit systemd d'exemple. `make agent-install-check` démarre un serveur et vérifie `/health`. Notes macOS / Windows courtes. Guide : [../docs/assist051_052_053_deploy.md](../docs/assist051_052_053_deploy.md). Pas d'installateur MSI. Pas de paquet distro officiel.

### ASSIST-052 - Image hyperviseur

**En tant que** operateur, **je veux** une image VM de l'instance Mohhdy, **afin de** l'isoler sur un hyperviseur.

**Dependances.** ASSIST-050. Distincte de l'ISO GRUB du prototype guest.

**Critère.** Image démarre, expose HTTP(S) admin / embed. Ne pas confondre avec `make iso` i386.

**Progrès.** Recette documentée : compose dans une VM Linux, ou QEMU/KVM x86_64 + stub cloud-init (`agent/packaging/cloud-init/`). `make agent-hypervisor-dry-run` valide les fichiers et imprime la commande, **sans** télécharger d'image et **sans** booter `mohhdy.bin`. Pas d'artefact qcow2 dans Git. Pas de Packer obligatoire.

### ASSIST-053 - Abonnement cloud hébergé

**En tant que** client, **je veux** une instance hébergée par abonnement, **afin d'**utiliser le support agent sans opérer le runtime.

**Dépendances.** ASSIST-050 au moins amorçable. OpenAI public toujours sous condition.

**Critère.** Provision d'une origine, snippet d'embed, quota documenté. Self-host reste possible. Hors CI publique : pas d'appel réseau payant.

**Progrès.** Scaffold, **pas** un SaaS de facturation. `MOHHDY_AGENT_MODE=self_host|hosted`, `MOHHDY_AGENT_SITE_ID`, bloc `quota` (toujours `billing=false`, `enforced=false`). `/health` et `/api/admin/status` exposent le mode, les locataires de config (`sites`) et le placeholder de quota. Self-host inchangé. Aucun Stripe / OpenAI public.

### ASSIST-060 - Accès navigateur après déploiement

**En tant que** opérateur, **je veux** ouvrir un navigateur sur l'instance déployée, **afin de** voir ce que l'agent voit et d'administrer hors du seul panneau.

**Dependances.** ASSIST-050. Voisinage phase 3. US-031 **non livre**. Le navigateur-OS est un devoir du SE, pas une option lointaine.

**Critère.** URL documentée vers un navigateur de l'instance ou un équivalent. Pas de "navigateur-OS" complet (US-031).

**Progrès.** `GET /browser` : page locale qui iframe `/demo-app` et sonde `/api/browser` (etat harness, `browser_engine=optional_not_installed` sur le slim, `phase3_complete=false`, `us031_complete=false`). Controle operateur optionnel : `POST /api/browser/navigate` (501 si Playwright absent). Pas d'Internet public hors allowlist. Guides : [../docs/assist060_061_browser.md](../docs/assist060_061_browser.md), [../docs/assist_playwright_optional.md](../docs/assist_playwright_optional.md). **Pas** US-031.

### ASSIST-061 - FS navigateur si lancement direct

**En tant que** operateur, **je veux** que, lance comme runtime navigateur, Mohhdy expose son systeme de fichiers navigateur, **afin d'**aligner l'instance sur le navigateur-OS du SE sans declarer US-031 livre.

**Dependances.** ASSIST-060. Spec phase 3 conservee comme cible, pas comme fait.

**Critère.** Spec + prototype **hors** guest AOS. ETAT_REEL ne doit pas être mis à jour tant que ce n'est pas observable.

**Progrès.** Prototype hors guest : `GET /api/browser/fs` list+read dans `demo/` (static) et `data/` (`MOHHDY_AGENT_DATA`). UI `/browser/fs`. `MOHHDY_AGENT_RUNTIME=docker|browser`. Traversal refuse (403). Lecture admin-gatee si `ADMIN_TOKEN`. Pas d'ecriture (`405 fs_read_only`). Phase 3 / US-031 restent **non implementes**. ETAT_REEL **non** mis a jour. Guide : [../docs/assist060_061_browser.md](../docs/assist060_061_browser.md).

### ASSIST-090 - Corps physique (futur)

**En tant que** opérateur futur, **je veux** que les mêmes capacités gouvernent un acte physique, **afin de** ne pas inventer un second modèle de droits le jour où un corps existe.

**Dépendances.** ASSIST-030 au minimum. **Aucune** livraison proche.

**Critère.** Paragraphe de piste dans ce fichier. Pas de ticket AOS, pas de matériel, pas de date.

## Ordre des épiques (résumé)

| Rang | IDs | Peut démarrer | Bloque par |
|---:|---|---|---|
| 1 | ASSIST-000 | Tout de suite (docs) | Rien |
| 2 | ASSIST-050 | Image Docker / HTTP livrée ; parallèle aux tranches AOS 0-4 | Interdit d'allonger la CI QEMU |
| 3 | ASSIST-010, 011, 013, 040 | Embed + sessions + admin a jeton + refus d'origine document : livrés (stub local ; pas d'auth par site) | Runtime plus large que i386 |
| 4 | ASSIST-012, 030, 031, 041 | KB locale, masque de droits, escalade, handoff dans la même conversation : livrés (stub) | Politique de droits |
| 5 | ASSIST-020, 021, 022 | Gestes simulateur, MCP déclaré, facture mock : livrés (pas Chromium) | Allowlist, pas AOS-025 public |
| 6 | ASSIST-051, 052, 053 | Après 050 amorçable | 051/052 docs+scripts livrés ; 053 scaffold non-billing |
| 7 | ASSIST-060, 061 | Apres 050 : `/browser` + FS sandbox livrés (pas US-031). Playwright = extra optionnel | Phase 3 reste spec |
| 8 | ASSIST-090 | Jamais en "prochain sprint" | Corps physique inexistant |

OpenAI / LLM hébergé : l'agent peut d'abord s'appuyer sur un modèle **local à l'instance** (voisinage AOS-010 / GGUF, autre runtime userspace). Un fournisseur public reste sous la **même** condition que le prototype : accord, secret hors image, hors CI.

## Liens

- Prototype guest verifie : [mohhdy_us.md](mohhdy_us.md), [../docs/ETAT_REEL.md](../docs/ETAT_REEL.md)
- Suite guest + capacites OS : [../docs/PLAN_SUITE_IMPLEMENTATION.md](../docs/PLAN_SUITE_IMPLEMENTATION.md)
- Scaffold Docker ASSIST-050 : [../docs/assist050_docker_runtime.md](../docs/assist050_docker_runtime.md)
- Packaging PC / hyperviseur / cloud : [../docs/assist051_052_053_deploy.md](../docs/assist051_052_053_deploy.md)
- Sessions / embed / admin : [../docs/assist010_sessions_admin.md](../docs/assist010_sessions_admin.md)
- Origine document embed : [../docs/assist013_origine_embed.md](../docs/assist013_origine_embed.md)
- KB, droits, escalade, handoff : [../docs/assist012_droits_handoff.md](../docs/assist012_droits_handoff.md)
- Gestes simulateur, MCP, facture : [../docs/assist020_gestes_mcp.md](../docs/assist020_gestes_mcp.md)
- Vue navigateur et FS sandbox : [../docs/assist060_061_browser.md](../docs/assist060_061_browser.md)
- Profil Playwright optionnel : [../docs/assist_playwright_optional.md](../docs/assist_playwright_optional.md)
- Phase 1 droits : [mohhdy_us_phase1_foundation.md](mohhdy_us_phase1_foundation.md)
- Phase 2 assistant : [mohhdy_us_phase2_ai_core.md](mohhdy_us_phase2_ai_core.md), [individual_us/US-021_Assistant_IA_Integre.md](individual_us/US-021_Assistant_IA_Integre.md), [individual_us/US-028_Module_Intelligence_Conversationnelle.md](individual_us/US-028_Module_Intelligence_Conversationnelle.md)
- Phase 3 navigateur : [mohhdy_us_phase3_web_runtime.md](mohhdy_us_phase3_web_runtime.md)
- Déploiement vision : [individual_us/US-015_Framework_Deploiement_Orchestration.md](individual_us/US-015_Framework_Deploiement_Orchestration.md)
- Connecteurs vision : [individual_us/US-034_Connecteurs_Systemes_Entreprise.md](individual_us/US-034_Connecteurs_Systemes_Entreprise.md)
