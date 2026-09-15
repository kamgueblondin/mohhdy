# Track Agent Support - assistant web agentique

**Date :** 15 septembre 2026
**Statut :** spec produit. ASSIST-050/010/011/040 = runtime HTTP livré (echo stub, sessions, admin a jeton) ; le reste du track n'est **pas** livré
**IDs :** `ASSIST-xxx` (ne collident ni avec `AOS-xxx` ni avec `US-xxx`)
**Ponctuation :** ASCII usuel et accents français uniquement

Ce document décrit une **nouvelle piste produit** : MOHHDY comme assistant IA agentique (OS / agent) capable de tenir un support client sur un site web, d'agir dans le périmètre autorisé, et de céder la main à un humain. Il n'annule pas le prototype i386. `agent/` sert une origine HTTP : widget `embed.js`, sessions visiteur isolées, console `/admin` (jeton `ADMIN_TOKEN` optionnel). Les réponses sont un **echo stub local**, pas un LLM de production, pas d'automatisation navigateur.

En cas de contradiction sur ce qui **tourne aujourd'hui**, [../docs/ETAT_REEL.md](../docs/ETAT_REEL.md) et [mohhdy_us.md](mohhdy_us.md) priment.

Plan d'ordre : [../docs/PLAN_SUITE_IMPLEMENTATION.md](../docs/PLAN_SUITE_IMPLEMENTATION.md). Index des couches : [README.md](README.md).

## Positionnement (sans réécrire l'histoire)

MOHHDY a deux couches déjà documentées, plus cette piste :

| Couche | Ce qu'elle dit | Ce qu'elle n'est pas |
|---|---|---|
| Prototype AOS | Hobby OS i386 Multiboot sous QEMU (AOS-001 à AOS-026 vérifiés) | Un runtime web, un widget JS, une image Docker |
| Vision phases 1-8 | Specs historiques (Foundation, AI Core, Web Runtime, etc.) | Un backlog de build du prototype |
| **Track Agent Support** | Produit "assistant qui agit" pour le support de site, avec déploiement Docker / PC / hyperviseur / cloud | Une fonction déjà livrée dans le noyau i386 |

Le widget JS, la console admin, l'agent d'actions navigateur et l'emballage Docker exigent un **runtime plus large** qu'un noyau Multiboot i386 nu. Cette piste ne prétend pas les faire tourner dans le shell Ring 3 actuel.

Vocabulaire **réutilisé**, sans nouveaux IDs vision :

- **Capacités / droits** : même idée que Foundation (grant, revoke, scope, moindre privilège). Les capabilities AOS actuelles sont locales, volatiles et liées à un PID Ring 3. Le track Agent Support a besoin d'un modèle équivalent **par site, par session et par outil**, pas d'un renumérotage de US-001 / US-003.
- **Assistant / IA** : voisinage de US-021 (assistant intégré) et US-028 (intelligence conversationnelle). Le prototype n'offre que `ai <texte>` synchrone et borné (AOS-010). Ce track produitise l'assistant pour le support client. Ce n'est **pas** TensorFlow Lite (US-016).
- **Navigateur comme FS** : voisinage de la phase 3 (US-031 à US-033 dans [mohhdy_us_phase3_web_runtime.md](mohhdy_us_phase3_web_runtime.md)). L'accès navigateur et l'exposition du système de fichiers navigateur sont une **tranche produit** de cette vision. La phase 3 reste **non implémentée**.
- **Déploiement** : voisinage de US-015 (framework de déploiement) et de la phase 6 / phase 8. Docker / cloud ici sont des cibles de **packaging d'agent**, pas l'orchestration microkernel complète.
- **Actions dans l'appli cliente** : voisinage de US-034 (connecteurs entreprise) et d'outils MCP / allowlist, pas un ERP livré.

Dépendances honnêtes vers le prototype :

- Les priorités AOS proches (budget CI, ACL préfixe, latence GGUF, migration stockage) **restent** les tranches 0-4. Ce track ne les remplace pas.
- OpenAI public reste **sous condition** (accord explicite, secret hors image, hors CI), y compris pour l'agent de support.
- Docker peut **commencer avant** un microkernel US-001 complet : le runtime agent n'a pas à attendre que ATA / FAT quittent le noyau i386.

## Vision

MOHHDY n'est plus seulement un prototype pédagogique i386. Il évolue en **assistant OS / agent** : une instance que l'opérateur déploie (Docker, PC, hyperviseur, ou abonnement cloud), que le visiteur d'un site rencontre via un embed JavaScript (UX proche de tawk.to), et qui **agit** au lieu de seulement discuter.

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

- Ne pas déclarer livrés le produit complet : LLM de production, hyperviseur, abonnement cloud, agent souris / clics, MCP site, facture en session, handoff humain. Le runtime HTTP (`agent/`) n'est pas ce produit.
- Ne pas déplacer le backlog AOS (CI 25 min, ACL préfixe, GGUF, stockage hors noyau) vers ce track.
- Ne pas héberger l'embed public **dans** le noyau Multiboot i386 actuel.
- Ne pas ouvrir TensorFlow Lite, NLU fédéré, P2P, économie de points, ou US-001 "d'un coup".
- Ne pas appeler un hôte OpenAI public depuis la CI, ni placer un secret dans le JS d'embed.
- Ne pas livrer un corps physique, ni un robot, dans les épiques proches.
- Ne pas inventer une troisième numérotation `US-xxx` ni recycler `AOS-xxx` pour le web.

## Matrice de déploiement

| Cible | Rôle | Runtime supposé | Statut |
|---|---|---|---|
| OS autonome existant | Prototype pédagogique i386, boot QEMU / ISO | Multiboot, shell Ring 3, GPT-2 / GGUF local, NE2000 local | **Vérifié** (AOS). N'héberge pas le widget |
| Conteneur Docker | Véhicule principal du runtime agent + origine de l'embed + admin | Userspace Linux (ou équivalent) avec HTTP(S), navigateur outillé, file de sessions | Runtime HTTP `agent/` : embed, sessions, admin a jeton. Pas de LLM de production ni d'actes navigateur |
| Installation PC | Même runtime, package natif | Identique à Docker sur le fond, installateur en plus | Spec ASSIST-051 |
| Hyperviseur | Image VM (QEMU/KVM, autre) du runtime agent, pas du seul hobby kernel | Identique à Docker, disque / réseau de VM | Spec ASSIST-052 |
| Abonnement cloud hébergé | Instance opérée pour le client, même API d'embed | Multi-tenant ou instance dédiée, facturation | Spec ASSIST-053, optionnelle |

L'utilisateur choisit : **son** Docker / cloud, ou l'offre hébergée. Les deux exposent le même contrat d'embed et de droits. L'OS i386 reste une cible autonome distincte : il peut, plus tard, dialoguer avec une instance agent, mais ce n'est pas le chemin pour coller un script sur un site marchand.

## Modèle de droits et de sécurité

Le prototype Foundation accorde déjà des droits backend VFS (`read` / `mutate` / `full`), un masque de sources, un préfixe interne, une révocation et un inventaire borné. Le track Agent Support **reprend cette discipline**, à une autre échelle.

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

Comportement (ASSIST-010 livré, actes navigateur non) :

1. Le script dessine un lanceur, puis un panneau de conversation.
2. Il ouvre une session visiteur vers l'origine de l'instance.
3. Les messages transitent en HTTP(S) vers `/api/sessions`. Les actes (clics, MCP) **ne sont pas** encore exécutés.
4. Les réponses sont un echo stub local, pas un LLM de production.
5. Un bouton "parler à un humain" n'existe pas encore (ASSIST-031).

Scripts d'intégration (ASSIST-013) : snippet et CSP dans [../docs/assist010_sessions_admin.md](../docs/assist010_sessions_admin.md). Le refus automatique d'origine document vs site déclaré reste ouvert (ASSIST-030).

## Console admin et handoff

La console n'est pas le shell VGA. C'est une UI web de l'instance.

Livré (ASSIST-040, lecture) : liste des sessions, détail du fil, jeton `ADMIN_TOKEN` optionnel.

Encore spec : filtres escalade / clos, actes et refus, reprise humaine (ASSIST-041).

- Reprise : l'humain écrit **dans la même session** ; l'agent se tait ou passe en observateur selon la politique.
- Interdit : fusionner des sessions de sites distincts, exporter un secret, agir sans `admin.takeover`.

## Agent d'actions navigateur

Objectif : l'agent ne se contente pas de répondre. Sur une origine autorisée, il peut cliquer, déplacer le pointeur, remplir un champ, appeler un outil MCP du site.

Exemple d'acte de session (ASSIST-022) : **créer une facture** dans l'application déjà utilisée par le client, avec la même session, si `mcp.invoice.create` (ou l'équivalent déclaré) est accordé. Sinon : expliquer le refus et proposer l'escalade.

Limites :

- Pas d'acte hors origine / hors allowlist.
- Pas d'acte irréversible (paiement, suppression massive) sans politique explicite ou humain.
- L'automatisation navigateur vit dans le runtime Docker / PC / cloud, pas dans le guest i386 AOS.

## Accès navigateur une fois déployé

Après déploiement de l'instance :

- L'opérateur ouvre l'admin et, s'il le souhaite, un navigateur de l'instance (ASSIST-060).
- Si MOHHDY est lancé "directement" comme runtime navigateur, il expose **son** système de fichiers navigateur (ASSIST-061). C'est le vocabulaire phase 3 "le navigateur devient le FS", **sans** déclarer US-031 livré.
- Les scripts d'attache (ASSIST-013) relient ce runtime à un site tiers pour le support.

## Modèle d'abonnement

Deux modes commerciaux, un contrat technique :

| Mode | Qui opère le runtime | Qui colle l'embed |
|---|---|---|
| Self-host | Le client (Docker, PC, hyperviseur) | Le client |
| Cloud hébergé | L'offre MOHHDY (abonnement) | Le client, vers l'origine hébergée |

L'abonnement ne remplace pas le self-host. Il ne rend pas le prototype i386 "SaaS". La facturation, le multi-tenant et le quota sont ASSIST-053. Ils dépendent d'un runtime Docker déjà amorçable, pas du microkernel.

## Corps physique (piste future)

ASSIST-090 : si MOHHDY dispose d'un corps, les actes physiques (déplacer un objet, tendre un document) réutilisent le même modèle de capacités et d'escalade. Aucune dépendance proche. Aucun matériel, aucun firmware, aucun planning de livraison dans les épiques 010-061.

## Épiques (user stories)

Convention : **En tant que** / **je veux** / **afin de**. Critère de sortie = observable. Statut implicite : **spec**.

### ASSIST-000 - Intent agentique (cadrage)

**En tant que** propriétaire produit, **je veux** que MOHHDY soit spécifié comme assistant OS / agent et pas seulement comme hobby i386, **afin de** pouvoir prioriser un runtime de support sans falsifier l'état AOS.

**Critère.** Ce fichier, [README.md](README.md) et [../docs/PLAN_SUITE_IMPLEMENTATION.md](../docs/PLAN_SUITE_IMPLEMENTATION.md) distinguent prototype vérifié et piste agent. Aucune phrase ne dit que l'embed tourne dans QEMU i386.

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

### ASSIST-013 - Scripts d'attache à un site tiers

**En tant que** opérateur, **je veux** un snippet et une checklist CSP / origine, **afin d'**attacher MOHHDY à n'importe quel site que je contrôle.

**Dépendances.** ASSIST-010.

**Critère.** Documentation plus snippet. Refus explicite si l'origine du document ne matche pas le site déclaré.

**Progrès (partiel).** Snippet et notes CSP dans [../docs/assist010_sessions_admin.md](../docs/assist010_sessions_admin.md). Pas de refus automatique d'origine (ASSIST-030).

### ASSIST-020 - Gestes navigateur autorisés

**En tant que** visiteur, **je veux** que l'agent clique ou saisisse **dans le périmètre autorisé**, **afin d'**avancer une tâche sans que je connaisse l'UI.

**Dépendances.** ASSIST-030, ASSIST-050 (navigateur outillé du runtime). **N'entre pas** dans le guest AOS.

**Critère.** Un scénario de démo (ouvrir un menu, remplir un champ) réussit sur l'origine allowlistée et échoue hors origine, avec journal `request_id`.

### ASSIST-021 - Outils MCP / outils du site

**En tant que** opérateur, **je veux** déclarer les outils du site (MCP ou équivalent) que l'agent a le droit d'appeler, **afin de** brancher MOHHDY sur mon appli sans lui donner la clef de tout.

**Dépendances.** ASSIST-030. Voisinage US-034, sans livrer un connecteur ERP générique.

**Critère.** Allowlist nominative. Un outil absent est refusé. La révocation empêche l'appel suivant.

### ASSIST-022 - Acte métier en session (exemple : facture)

**En tant que** visiteur autorisé, **je veux** que l'agent crée une facture **dans l'application déjà en place**, **afin de** terminer l'acte sans changer d'outil ni de session.

**Dépendances.** ASSIST-021, droit `mcp.invoice.create` (ou nom déclaré). Humain si la politique l'exige.

**Critère.** Une facture de démo apparaît côté appli hôte quand le droit est là ; sinon message de refus + offre d'escalade. Même `session_id`.

### ASSIST-030 - Droits par site et par session

**En tant que** opérateur, **je veux** accorder, limiter et révoquer des capacités par site et par session, **afin que** l'agent n'agisse que dans le périmètre voulu.

**Dépendances.** Vocabulaire Foundation (grant / revoke / scope). **Pas** besoin que US-001 soit terminé. Identité vérifiée Foundation reste un plus, pas un préalable Docker.

**Critère.** Masque de droits visible côté admin. Diagnostic public du widget sans secret ni borne interne. Preuves négatives (outil retiré, origine étrangère).

### ASSIST-031 - Escalade humaine

**En tant que** visiteur, **je veux** un humain quand je le demande, ou quand la politique l'impose, **afin de** ne pas rester bloqué face à un agent.

**Dépendances.** ASSIST-011, ASSIST-040.

**Critère.** Demande visiteur : file admin. Tentative d'acte hors politique : file admin, sans exécution. L'agent n'invente pas un droit pour "aider quand même".

### ASSIST-040 - Console d'administration des sessions

**En tant que** humain de support, **je veux** lister et relire les échanges, **afin de** contrôler ce que l'agent a dit et fait.

**Dépendances.** ASSIST-050 ou ASSIST-053, ASSIST-011.

**Critère.** Liste, filtre ouvert / escalade / clos, détail d'une session, actes et refus. Authentification opérateur.

**Progrès (basique).** `/admin` liste et relit les fils. Si `ADMIN_TOKEN` est défini à l'exécution, `/api/admin/*` exige `Authorization: Bearer` (jamais cuit dans l'image). Sans variable : mode stub ouvert, bandeau d'avertissement. Pas de comptes, pas de filtres escalade/clos, pas de handoff (ASSIST-041).

### ASSIST-041 - Handoff : l'humain continue

**En tant que** humain de support, **je veux** écrire dans la **même** conversation, **afin que** le visiteur n'ait pas à se répéter.

**Dépendances.** ASSIST-040, ASSIST-031.

**Critère.** Après takeover, les messages humains apparaissent dans le widget. L'agent ne répond plus (ou seulement en observateur, selon politique). Journal de qui parle.

### ASSIST-050 - Image Docker du runtime agent

**En tant que** opérateur d'instance, **je veux** un conteneur qui sert l'embed, l'admin et l'agent, **afin de** déployer MOHHDY sans attendre un microkernel i386.

**Dépendances.** Aucune tranche AOS 0-4 bloquante. **Peut commencer en parallèle.** Ne pas alourdir `make integration-qemu`.

**Critère.** `docker run` documenté : santé HTTP, page admin, origine d'embed. Pas de secret dans l'image. Hors cible : faire booter le noyau Multiboot **dans** ce conteneur comme substitut du widget.

**Progrès.** Arborescence `agent/` : Python 3 stdlib, `Dockerfile` utilisateur non-root, `docker-compose.yml`, `make agent-smoke` (hors `make ci` / hors QEMU). Sert embed, sessions, admin. Guide image : [../docs/assist050_docker_runtime.md](../docs/assist050_docker_runtime.md). **Non livré dans l'image :** secret, LLM de production, MCP, actes navigateur, noyau i386.

### ASSIST-051 - Installation PC

**En tant que** opérateur, **je veux** installer le même runtime sur un PC, **afin de** m'en servir sans Docker si je le préfère.

**Dépendances.** ASSIST-050 (même contrat). Voisinage phase 6 / US-015.

**Critère.** Paquet ou installateur documenté, même API d'embed que Docker.

### ASSIST-052 - Image hyperviseur

**En tant que** opérateur, **je veux** une image VM du runtime agent, **afin de** l'isoler sur un hyperviseur.

**Dépendances.** ASSIST-050. Distincte de l'ISO GRUB du prototype AOS.

**Critère.** Image démarre, expose HTTP(S) admin / embed. Ne pas confondre avec `make iso` i386.

### ASSIST-053 - Abonnement cloud hébergé

**En tant que** client, **je veux** une instance hébergée par abonnement, **afin d'**utiliser le support agent sans opérer le runtime.

**Dépendances.** ASSIST-050 au moins amorçable. OpenAI public toujours sous condition.

**Critère.** Provision d'une origine, snippet d'embed, quota documenté. Self-host reste possible. Hors CI publique : pas d'appel réseau payant.

### ASSIST-060 - Accès navigateur après déploiement

**En tant que** opérateur, **je veux** ouvrir un navigateur sur l'instance déployée, **afin de** voir ce que l'agent voit et d'administrer hors du seul panneau.

**Dépendances.** ASSIST-050. Voisinage phase 3, **non livré**.

**Critère.** URL documentée vers un navigateur de l'instance ou un équivalent. Pas de "navigateur-OS" complet (US-031).

### ASSIST-061 - FS navigateur si lancement direct

**En tant que** opérateur, **je veux** que, lancé comme runtime navigateur, MOHHDY expose son système de fichiers navigateur, **afin d'**aligner le produit sur la vision "le web est le FS" sans réécrire la phase 3 comme faite.

**Dépendances.** ASSIST-060. Spec phase 3 conservée.

**Critère.** Spec + prototype **hors** guest AOS. ETAT_REEL ne doit pas être mis à jour tant que ce n'est pas observable.

### ASSIST-090 - Corps physique (futur)

**En tant que** opérateur futur, **je veux** que les mêmes capacités gouvernent un acte physique, **afin de** ne pas inventer un second modèle de droits le jour où un corps existe.

**Dépendances.** ASSIST-030 au minimum. **Aucune** livraison proche.

**Critère.** Paragraphe de piste dans ce fichier. Pas de ticket AOS, pas de matériel, pas de date.

## Ordre des épiques (résumé)

| Rang | IDs | Peut démarrer | Bloque par |
|---:|---|---|---|
| 1 | ASSIST-000 | Tout de suite (docs) | Rien |
| 2 | ASSIST-050 | Image Docker / HTTP livrée ; parallèle aux tranches AOS 0-4 | Interdit d'allonger la CI QEMU |
| 3 | ASSIST-010, 011, 013, 040 | Embed + sessions + admin a jeton livrés (echo stub ; CSP docs ; pas d'auth par site) | Runtime plus large que i386 |
| 4 | ASSIST-012, 030, 031, 041 | Après session + admin | Politique de droits |
| 5 | ASSIST-020, 021, 022 | Après droits + navigateur outillé | Allowlist, pas AOS-025 public |
| 6 | ASSIST-051, 052, 053 | Après 050 amorçable | 053 : opérateur cloud |
| 7 | ASSIST-060, 061 | Après 050 | Phase 3 reste spec |
| 8 | ASSIST-090 | Jamais en "prochain sprint" | Corps physique inexistant |

OpenAI / LLM hébergé : l'agent peut d'abord s'appuyer sur un modèle **local à l'instance** (voisinage AOS-010 / GGUF, autre runtime userspace). Un fournisseur public reste sous la **même** condition que le prototype : accord, secret hors image, hors CI.

## Liens

- Prototype vérifié : [mohhdy_us.md](mohhdy_us.md), [../docs/ETAT_REEL.md](../docs/ETAT_REEL.md)
- Suite AOS + ce track : [../docs/PLAN_SUITE_IMPLEMENTATION.md](../docs/PLAN_SUITE_IMPLEMENTATION.md)
- Scaffold Docker ASSIST-050 : [../docs/assist050_docker_runtime.md](../docs/assist050_docker_runtime.md)
- Sessions / embed / admin : [../docs/assist010_sessions_admin.md](../docs/assist010_sessions_admin.md)
- Phase 1 droits : [mohhdy_us_phase1_foundation.md](mohhdy_us_phase1_foundation.md)
- Phase 2 assistant : [mohhdy_us_phase2_ai_core.md](mohhdy_us_phase2_ai_core.md), [individual_us/US-021_Assistant_IA_Integre.md](individual_us/US-021_Assistant_IA_Integre.md), [individual_us/US-028_Module_Intelligence_Conversationnelle.md](individual_us/US-028_Module_Intelligence_Conversationnelle.md)
- Phase 3 navigateur : [mohhdy_us_phase3_web_runtime.md](mohhdy_us_phase3_web_runtime.md)
- Déploiement vision : [individual_us/US-015_Framework_Deploiement_Orchestration.md](individual_us/US-015_Framework_Deploiement_Orchestration.md)
- Connecteurs vision : [individual_us/US-034_Connecteurs_Systemes_Entreprise.md](individual_us/US-034_Connecteurs_Systemes_Entreprise.md)
