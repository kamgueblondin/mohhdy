/* osui_parity.c - the family table of the GUI parity panels (see
 * osui_parity.h). Pure data plus three helpers, host testable. */
#include "osui_parity.h"
#include "../include/os_syscalls.h"

#define N(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define A OSUI_PAR_ARG
#define M OSUI_PAR_MUT

static const osui_par_entry_t k_net[] = {
    { "Etat reseau (pilote, IP, liens)", "net-status", 0, 0 },
    { "Fil TCP noyau (connexions, paquets)", "net-wire-status", 0, 0 },
    { "Relais reseau (forwards, refus)", "net-relay-status", 0, 0 },
    { "Sante P2P", "p2p-health", 0, 0 },
    { "Pairs P2P", "p2p-peers", 0, 0 },
    { "Monter le P2P", "p2p-up", A | M, "NOM IP [CLE]" },
    /* Lot 47 (US-G04): same `net-if` command as the console */
    { "Interface (adresse, masque, passerelle)", "net-if", 0, 0 },
    { "Configurer l'interface", "net-if", A | M, "static IP MASQUE PASSERELLE | dhcp" },
};
static const osui_par_entry_t k_store[] = {
    { "Pilote ATA (lectures, ecritures)", "ata-status", 0, 0 },
    { "Points de montage", "mount-journal", 0, 0 },
    { "Persistance (blobs)", "persist-status", 0, 0 },
    { "Lister un repertoire", "ls", A, "CHEMIN (ex: /samples)" },
    { "Infos d'un fichier", "stat", A, "CHEMIN" },
    { "Creer un repertoire", "mkdir", A | M, "CHEMIN" },
    /* Lot 46: the rest of the storage commands of the shell */
    { "Sauver la persistance (noeud chiffre)", "persist-save", M, 0 },
    { "Lister le volume FAT16", "fat16-list", 0, 0 },
};
static const osui_par_entry_t k_services[] = {
    { "Pilotes en ring 3 (ps2, serie, vga, usb)", "devices", 0, 0 },
    { "Bus PCI", "lspci", 0, 0 },
    { "Peripheriques (inventaire)", "dev-list", 0, 0 },
    { "Taches en arriere-plan", "jobs", 0, 0 },
    { "Resume des taches", "task-summary", 0, 0 },
    { "Redemarrer un pilote", "driver-restart", A | M, "NOM (ex: atadriver, networker)" },
    { "Etat d'un service", "service-status", A, "NOM (ex: vfs)" },
    { "Trouver un service", "service-find", A, "NOM" },
};
static const osui_par_entry_t k_logs[] = {
    { "Evenements des taches", "task-events", 0, 0 },
    { "Notifications", "notify-list", 0, 0 },
    { "Audit des actions IA", "osui-audit", 0, 0 },
    { "Historique des commandes", "history", 0, 0 },
    { "Usage des commandes", "prod-usage", 0, 0 },
    { "Analyser un journal", "prod-log-analyze", A, "FICHIER" },
    { "Ajouter au journal", "prod-log-append", A | M, "FICHIER TEXTE" },
    /* Lot 46: notifications, push and acknowledge */
    { "Pousser une notification", "notify-push", A | M, "PRIORITE(1-3) TEXTE" },
    { "Acquitter une notification", "notify-ack", A | M, "ID" },
};
static const osui_par_entry_t k_ledger[] = {
    { "Soldes du registre", "collab-balances", 0, 0 },
    { "Taches collaboratives", "collab-tasks", 0, 0 },
    { "Journal d'audit", "collab-audit", 0, 0 },
    { "Monter le P2P (requis)", "p2p-up", A | M, "NOM IP [CLE]" },
    { "Formulaire: publier une offre", "collab-offer", A | M, "NOM PRIX CAPACITE" },
    { "Formulaire: voter", "collab-vote", A | M, "PROPOSANT SEQ yes|no" },
    { "Formulaire: creer un escrow", "collab-escrow-create", A | M, "BENEFICIAIRE ARBITRE|none MONTANT [NOM]" },
    { "Formulaire: solder / litige", "collab-escrow-release", A | M, "PAYEUR SEQ" },
};
static const osui_par_entry_t k_prod[] = {
    { "Diagnostic de production", "prod-diag", 0, 0 },
    { "Alertes", "prod-alerts", 0, 0 },
    { "Sauvegardes", "prod-backups", 0, 0 },
    { "Mise a l'echelle", "prod-scale-status", 0, 0 },
    { "Metriques", "prod-metrics", 0, 0 },
    { "Prendre un echantillon", "prod-sample", M, 0 },
    /* Lot 46: deploy-verify needs its manifest (it always answered
     * "manifest not found" without one); the manifest is built here too */
    { "Verifier le deploiement", "deploy-verify", A, "MANIFESTE (ex: /deploy.man)" },
    { "Integrite d'un repertoire", "prod-integrity", A, "baseline|check DIR" },
    { "Construire un manifeste", "deploy-make", A | M, "/SOURCE /CIBLE MANIFESTE" },
    { "Sauvegarder un repertoire", "prod-backup", A | M, "NOM DIR" },
};
static const osui_par_entry_t k_security[] = {
    { "Identite (whoami)", "whoami", 0, 0 },
    { "Cles d'identite", "id-key", 0, 0 },
    { "Origine (politique)", "origin-check", 0, 0 },
    { "Verifier une signature CA", "pm-ca-verify", A, "FICHIER.pm (ex: /pm/hello.pm)" },
    { "Initialiser la CA locale", "pm-ca-init", M, 0 },
    { "Signer un fichier", "pm-ca-sign", A | M, "FICHIER" },
    /* Lot 46: local revocation list */
    { "Revoquer une signature", "pm-ca-revoke", A | M, "FICHIER.pm" },
};
static const osui_par_entry_t k_search[] = {
    { "Chercher dans un repertoire (recursif)", "grep -r", A, "MOTIF [REPERTOIRE] [PAGE]" },
    { "Chercher dans un fichier", "grep", A, "MOTIF FICHIER" },
    { "Lister un repertoire", "ls", A, "CHEMIN" },
};
static const osui_par_entry_t k_devtools[] = {
    { "Etat du navigateur", "browser-status", 0, 0 },
    { "Onglets", "browser-tabs", 0, 0 },
    { "Performance web", "web-perf", 0, 0 },
    { "Cache web", "web-cache", 0, 0 },
    { "Arbre DOM d'un fichier", "web-dom", A, "FICHIER (ex: /web/demo.html)" },
    { "Console d'un fichier", "web-console", A, "FICHIER" },
    /* Lot 46: inspector and accessibility audit */
    { "Inspecter un element", "web-inspect", A, "FICHIER ID" },
    { "Audit d'accessibilite", "web-a11y", A, "FICHIER" },
};
static const osui_par_entry_t k_notes[] = {
    { "Lister la racine (notes de l'overlay)", "ls /", 0, 0 },
    { "Lire une note", "cat", A, "FICHIER (ex: /notes.txt)" },
    { "Ajouter une ligne a une note", "append", A | M, "FICHIER TEXTE (ex: /notes.txt)" },
    { "Archiver un repertoire", "migrate-export", A | M, "DIR ARCHIVE" },
    { "Verifier une archive", "migrate-verify", A, "ARCHIVE [DIR]" },
    /* Lot 46: restore an archive */
    { "Restaurer une archive", "migrate-import", A | M, "ARCHIVE DIR" },
};
static const osui_par_entry_t k_vfs[] = {
    { "Statistiques VFS", "vfs-stats", 0, 0 },
    { "Lister un repertoire VFS", "vfs-list", A, "REPERTOIRE/ (ex: /samples/)" },
    { "Metadonnees VFS", "vfs-stat", A, "CHEMIN" },
    { "Lire via VFS", "vfs-read", A, "CHEMIN" },
    { "Ajouter un montage VFS", "vfs-mount-add", A | M, "PREFIXE/ SOURCE" },
    { "Retirer un montage VFS", "vfs-mount-remove", A | M, "PREFIXE/" },
};
static const osui_par_entry_t k_sessions[] = {
    { "Statut de la session", "session-status", 0, 0 },
    { "Lister les sessions", "session-list", 0, 0 },
    { "Nouvelle session", "session-new", A | M, "[SITE]" },
    { "Utiliser une session", "session-use", A | M, "ID" },
    { "Restaurer une session", "session-restore", A | M, "ID" },
    { "Nettoyer les sessions expirees", "session-cleanup", M, 0 },
    { "Fermer la session", "session-end", M, 0 },
};
static const osui_par_entry_t k_mcp[] = {
    { "Statut MCP", "mcp-status", 0, 0 },
    { "Lister les services MCP", "mcp-list", 0, 0 },
    { "Identifiants MCP", "mcp-credentials", 0, 0 },
    { "Invoquer un outil MCP", "mcp-invoke", A | M, "OUTIL" },
    { "Facturer un client MCP", "mcp-invoice", A | M, "CLIENT MONTANT" },
    { "Annuler une facture MCP", "mcp-invoice-void", A | M, "FACTURE_ID" },
};
static const osui_par_entry_t k_media[] = {
    { "Etat du lecteur media (simule)", "browser-media", 0, 0 },
    { "Lire un media (simule)", "media-play", A | M, "[TYPE] [TITRE]" },
    { "Mettre en pause le flux", "media-pause", M, 0 },
    { "Arreter la lecture media", "media-stop", M, 0 },
    { "Diffuser un flux (simule)", "media-stream", A | M, "[URL] [TYPE]" },
};
static const osui_par_entry_t k_grpc[] = {
    { "Appel gRPC wire", "grpc-wire", A, "SERVICE METHOD [PAYLOAD]" },
};
static const osui_par_entry_t k_pwa[] = {
    { "Lister les PWA installees", "pwa-list", 0, 0 },
    { "Declarer un manifeste PWA", "pwa-manifest", A | M, "NOM URL [SCOPE]" },
    { "Installer une PWA", "pwa-install", A | M, "NOM" },
    { "Lancer une PWA", "pwa-run", A | M, "NOM" },
};
static const osui_par_entry_t k_devsync[] = {
    { "Afficher un appareil devsync", "devsync-show", A, "A|B|C" },
    { "Synchroniser deux appareils", "devsync-sync", A | M, "SRC DST (A|B|C)" },
    { "Ecrire une cle devsync", "devsync-set", A | M, "A|B|C CLE VALEUR" },
    { "Supprimer une cle devsync", "devsync-del", A | M, "A|B|C CLE" },
};
static const osui_par_entry_t k_admin[] = {
    { "Administration OS", "os-admin", 0, 0 },
    { "Statut OS", "os-status", 0, 0 },
    { "Statut administrateur", "admin-status", 0, 0 },
    { "Tous les parametres admin", "admin-all", 0, 0 },
    { "Support OS", "os-support", 0, 0 },
};
static const osui_par_entry_t k_api[] = {
    { "Apercu des API", "api", 0, 0 },
    { "Creer un token API", "api-token", M, 0 },
    { "Revoquer le token API", "api-revoke", M, 0 },
};

static const osui_par_entry_t k_help[] = {
    { "Programmes slash du bureau", "gui-help", 0, 0 },
    { "Commandes du terminal (help)", "help", 0, 0 },
};

/* Lot 50: terminal families that were only reachable through /cmd.
 * pm-ide stays terminal-only (keyboard loop). The mesh panel does not
 * bring the node up; a down node prints the shell's own refusal. */
static const osui_par_entry_t k_pm[] = {
    { "Catalogue des programmes", "pm-catalog", 0, 0 },
    { "Chercher dans le catalogue", "pm-catalog-search", A, "MOTIF" },
    { "Documentation d'un programme", "pm-doc", A, "FICHIER.pm (ex: /pm/hello.pm)" },
    { "Verifier un programme", "pm-check", A, "FICHIER.pm" },
    { "Desassembler", "pm-disasm", A, "FICHIER.pm" },
    { "Aide a l'ecriture (regles)", "pm-assist", A, "FICHIER.pm" },
    { "Traduire (C99 ou Python)", "pm-cross", A, "FICHIER [py]" },
    { "Tester", "pm-test", A, "FICHIER.pm" },
    { "Versions", "pm-versions", A, "FICHIER.pm" },
    /* Lot 65 (US-G19): the rest of the pm-* family. pm-ide and pm-edit
     * are the line editor (keyboard loop): terminal-only. */
    { "Version d'un programme", "pm-version", A, "FICHIER.pm" },
    { "Executer un programme", "pm-run", A, "FICHIER.pm" },
    { "Tracer / point d'arret", "pm-debug", A, "FICHIER.pm [LIGNE]" },
    { "Dire une phrase au programme charge", "pm-say", A, "PHRASE" },
    { "Compiler en image PMC1", "pm-compile", A | M, "SOURCE SORTIE" },
    { "Executer une image PMC1", "pm-exec", A, "IMAGE" },
    { "Installer depuis le catalogue", "pm-install", A | M, "NOM" },
    { "Recuperer du catalogue", "pm-catalog-fetch", A | M, "NOM" },
    { "Certifier un programme", "pm-certify", A | M, "FICHIER.pm" },
    { "Verifier un certificat", "pm-verify", A, "FICHIER.pm" },
    { "Creer la CA locale", "pm-ca-init", M, 0 },
    { "Etat du scellement de la CA", "pm-ca-seal", 0, 0 },
    { "Signer avec la CA", "pm-ca-sign", A | M, "FICHIER.pm" },
    { "Verifier une signature CA", "pm-ca-verify", A, "FICHIER.pm" },
    { "Revoquer un certificat", "pm-ca-revoke", A | M, "FICHIER.pm" },
};
static const osui_par_entry_t k_proc[] = {
    { "Liste des processus", "ps", 0, 0 },
    { "Memoire", "mem", 0, 0 },
    { "Capacite des taches", "task-capacity", 0, 0 },
    { "Evenements des taches", "task-events", 0, 0 },
    { "Etat des notifications", "task-events-notify-status", 0, 0 },
    { "Priorite de supervision", "task-priority-child-status", 0, 0 },
    { "Budget des evenements", "task-events-budget-status", 0, 0 },
    { "Surveillance des evenements", "task-events-watch-status", 0, 0 },
    { "Enfants directs", "children", 0, 0 },
    { "Changer la priorite", "task-priority", A | M, "PID 1|2|3" },
    { "Suspendre un enfant", "task-suspend", A | M, "PID" },
    { "Reprendre un enfant", "task-resume", A | M, "PID" },
    /* Lot 65 (US-G20): every other task-* command. wait, wait-result and
     * wait-any-result block until a child exits: they stay on /cmd. */
    { "Resume des taches", "task-summary", 0, 0 },
    { "Metriques d'une tache", "task-metrics", A, "PID" },
    { "Un evenement", "task-event", A, "SEQUENCE" },
    { "Rejouer un evenement", "task-event-replay", A, "SEQUENCE" },
    { "Observer une generation", "task-events-observe", A, "GENERATION" },
    { "Statistiques de notification", "task-events-notify-stats", 0, 0 },
    { "Nommer une tache", "task-name", A | M, "PID NOM" },
    { "Deleguer un enfant", "task-delegate", A | M, "PID_ENFANT PID_SUPERVISEUR" },
    { "Priorite d'un enfant supervise", "task-priority-child", A | M, "PID|off" },
    { "Notifications on/off", "task-events-notify", A | M, "on|off" },
    { "Filtre des evenements", "task-events-filter", A | M, "all|exit|suspend|resume|delegate-out|delegate-in|none" },
    { "Budget des evenements", "task-events-budget", A | M, "N|off" },
    { "Surveiller une tache", "task-events-watch", A | M, "PID" },
    { "Ne plus surveiller", "task-events-unwatch", A | M, "PID" },
    { "Vider la surveillance", "task-events-watch-clear", M, 0 },
    { "Oublier un evenement", "task-events-forget", A | M, "SEQUENCE" },
    { "Vider les evenements", "task-events-clear", M, 0 },
    { "Vider les stats de notification", "task-events-notify-stats-clear", M, 0 },
    { "Arreter une tache", "kill", A | M, "PID" },
    { "Arreter les enfants", "kill-children", M, 0 },
};
static const osui_par_entry_t k_p2pmesh[] = {
    { "Statistiques du maillage", "p2p-stats", 0, 0 },
    { "Analyse", "p2p-analyze", 0, 0 },
    { "Routes", "p2p-route", 0, 0 },
    { "Qualite de service", "p2p-qos", 0, 0 },
    { "Etat Raft", "p2p-raft", 0, 0 },
    { "Etat STUN", "p2p-stun", 0, 0 },
    { "Etat du cache", "p2p-cache", 0, 0 },
    { "Magasin cle-valeur", "p2p-kv", 0, 0 },
    { "Ecrire dans le cache", "p2p-cache", A | M, "put CLE VALEUR" },
    /* Lot 65 (US-G21): the node itself and every other p2p-* command, so
     * the mesh is live from the panel. p2p-chat is a keyboard loop:
     * terminal-only. */
    { "Monter le noeud", "p2p-up", A | M, "NOM IP [CLE]" },
    { "Arreter le noeud", "p2p-down", M, 0 },
    { "Pairs", "p2p-peers", 0, 0 },
    { "Sante", "p2p-health", 0, 0 },
    { "Recevoir (poll)", "p2p-poll", A, "SECONDES" },
    { "Envoyer un message", "p2p-send", A | M, "PAIR TEXTE" },
    { "Ecrire une cle repliquee", "p2p-put", A | M, "CLE VALEUR" },
    { "Lire une cle", "p2p-get", A, "CLE" },
    { "Synchroniser avec un pair", "p2p-sync", A | M, "PAIR" },
    { "Proposer (consensus)", "p2p-propose", A | M, "CLE VALEUR" },
    { "Ecrire par Raft", "p2p-raft-put", A | M, "CLE VALEUR" },
    { "Compacter le journal Raft", "p2p-raft-compact", M, 0 },
    { "Lire le cache", "p2p-cache get", A, "CLE" },
    { "Invalider (versionne, acquitte)", "p2p-cache inv", A | M, "CLE [VERSION]" },
    { "Remplir avec une version", "p2p-cache putv", A | M, "CLE VALEUR VERSION" },
    { "Ecrire par le journal Raft", "p2p-cache raft", A | M, "CLE VALEUR" },
    { "Sonde STUN vers un pair", "p2p-stun", A | M, "PAIR" },
    { "Percage NAT (simule)", "p2p-punch", A | M, "PAIR" },
    { "Appel gRPC sur le maillage", "p2p-grpc", A | M, "PAIR SERVICE METHODE [CHARGE]" },
    { "Limite de debit", "p2p-limit", A | M, "N (0: aucune)" },
    { "Rafale lissee", "p2p-burst", A | M, "PAIR N [TEXTE]" },
    { "Inondation (test d'analyse)", "p2p-flood", A | M, "PAIR N" },
    { "Bloquer un pair", "p2p-block", A | M, "PAIR" },
    { "Debloquer un pair", "p2p-unblock", A | M, "PAIR" },
};

/* Lot 51: commands that were only reachable through /cmd.
 * /child does not take /p2p. /release does not take /prod or /ci.
 * /keys does not take /ledger or /collab. */
static const osui_par_entry_t k_child[] = {
    { "Nombre de sorties", "child-exit-count", 0, 0 },
    { "Historique des resultats", "child-results", 0, 0 },
    { "Dernier resultat d'un pid", "child-result", A, "PID" },
    { "Resultat retenu d'un pid", "child-result-any", A, "PID" },
    { "Observer une generation", "child-results-observe", A, "GENERATION" },
    { "Acquitter l'historique", "child-results-clear", M, 0 },
    { "Oublier un resultat", "child-results-forget", A | M, "PID" },
};
static const osui_par_entry_t k_release[] = {
    { "Verifier un manifeste", "deploy-verify", 0, 0 },
    { "Appliquer un manifeste", "deploy-apply", A | M, "MANIFESTE" },
    { "Revenir au backup pre-deploy", "prod-rollback", M, 0 },
    { "Restaurer un backup nomme", "prod-restore", A | M, "NOM" },
};
static const osui_par_entry_t k_keys[] = {
    { "Cles connues du registre", "collab-keys", 0, 0 },
    { "Deployer un contrat", "collab-contract-deploy", A | M, "BENEFICIAIRE ARBITRE|none MONTANT PROGRAMME" },
    /* Lot 65 (US-G24): every collab-* command not in /ledger. collab-forge
     * is the forged-transfer attack hook of the tests: it stays on /cmd. */
    { "Offres", "collab-offers", 0, 0 },
    { "Votes", "collab-votes", 0, 0 },
    { "Escrows et contrats", "collab-escrows", 0, 0 },
    { "Encheres", "collab-auctions", 0, 0 },
    { "Tickets", "collab-tickets", 0, 0 },
    { "Exporter le registre", "collab-export", 0, 0 },
    { "Rejoindre le registre", "collab-join", M, 0 },
    { "Resynchroniser", "collab-sync", M, 0 },
    { "Payer", "collab-pay", A | M, "PAIR MONTANT" },
    { "Reserver une offre", "collab-reserve", A | M, "VENDEUR SEQ" },
    { "Publier une tache", "collab-task", A | M, "PRIX sum N|primes N|fnv TEXTE" },
    { "Prendre une tache", "collab-claim", A | M, "AUTEUR SEQ" },
    { "Calculer une tache", "collab-work", A | M, "AUTEUR SEQ" },
    { "Verifier un resultat", "collab-review", A | M, "SEQ" },
    { "Noter un pair", "collab-rate", A | M, "PAIR NOTE" },
    { "Proposer", "collab-propose", A | M, "TEXTE" },
    { "Profil", "collab-profile", A | M, "show PAIR | set CLE VALEUR shared|private" },
    { "Ouvrir un ticket", "collab-ticket", A | M, "TEXTE" },
    { "Repondre a un ticket", "collab-answer", A | M, "AUTEUR SEQ TEXTE" },
    { "Signer un contrat", "collab-signoff", A | M, "PAYEUR SEQ" },
    { "Rembourser un escrow", "collab-escrow-refund", A | M, "PAYEUR SEQ" },
    { "Ouvrir un litige", "collab-dispute-open", A | M, "PAYEUR SEQ PREUVE" },
    { "Voter un litige", "collab-dispute-vote", A | M, "PAYEUR SEQ payer|payee" },
    { "Ouvrir une enchere", "collab-auction", A | M, "OBJET RESERVE" },
    { "Encherir", "collab-market-bid", A | M, "VENDEUR SEQ MONTANT" },
    { "Clore une enchere", "collab-auction-close", A | M, "SEQ" },
    { "Oublier mes entrees locales", "collab-forget", M, 0 },
};

const osui_par_builtin_t osui_par_builtins[] = {
    { "/tasks", OS_FB_APP_TASKS, "Gestionnaire des taches" },
    { "/view", OS_FB_APP_VIEW, "Visionneuse FICHIER (pgm ppm bmp png jpg)" },
    { "/settings", OS_FB_APP_SETTINGS, "Parametres" },
    { "/peers", OS_FB_APP_P2P, "Pairs P2P" },
    { "/pmrun", OS_FB_APP_PMRUN, "Lancer FICHIER.pm" },
    { "/chat", OS_FB_APP_CHAT, "Chat IA (ai-model)" },
    { "/files", 0, "Fichiers [CHEMIN]" },
    { "/edit", 0, "Editeur FICHIER" },
    { "/browse", 0, "Site local /web (aussi: web FICHIER)" },
    { "/shell", 0, "Terminal (aussi: !cmd depuis le chat)" },
    { "/console", 0, "Retour au mode texte" },
    { "/browser /fs /admin /support /status /center", 0, "Volets du bureau" },
};
const int osui_par_builtin_count = N(osui_par_builtins);

const osui_par_family_t osui_par_families[] = {
    { OS_FB_APP_HELP, "/help", "Aide", "help", k_help, N(k_help) },
    { OS_FB_APP_NET, "/net", "Reseau", "network", k_net, N(k_net) },
    { OS_FB_APP_STORE, "/disk", "Stockage", "storage", k_store, N(k_store) },
    { OS_FB_APP_SERVICES, "/services", "Services et pilotes", "services", k_services, N(k_services) },
    { OS_FB_APP_LOGS, "/logs", "Journaux", "logs", k_logs, N(k_logs) },
    { OS_FB_APP_LEDGER, "/ledger", "Registre collab", "collab", k_ledger, N(k_ledger) },
    { OS_FB_APP_PROD, "/ci", "Production", "prod", k_prod, N(k_prod) },
    { OS_FB_APP_SECURITY, "/security", "Securite et CA", "security", k_security, N(k_security) },
    { OS_FB_APP_SEARCH, "/search", "Recherche", "search", k_search, N(k_search) },
    { OS_FB_APP_DEVTOOLS, "/devtools", "Outils web", "web", k_devtools, N(k_devtools) },
    { OS_FB_APP_NOTES, "/notes", "Notes et archives", "notes", k_notes, N(k_notes) },
    { OS_FB_APP_VFS, "/vfs", "Virtuel VFS", "vfs", k_vfs, N(k_vfs) },
    { OS_FB_APP_SESSIONS, "/sessions", "Sessions utilisateur", "sessions", k_sessions, N(k_sessions) },
    { OS_FB_APP_MCP, "/mcp", "Paiements MCP", "mcp", k_mcp, N(k_mcp) },
    { OS_FB_APP_MEDIA, "/media", "Media et streaming", "media", k_media, N(k_media) },
    { OS_FB_APP_GRPC, "/grpc", "Wire gRPC", "grpc", k_grpc, N(k_grpc) },
    { OS_FB_APP_PWA, "/pwa", "Applications PWA", "pwa", k_pwa, N(k_pwa) },
    { OS_FB_APP_DEVSYNC, "/devsync", "DevSync cluster", "devsync", k_devsync, N(k_devsync) },
    { OS_FB_APP_ADMIN, "/sysadmin", "Administration", "admin", k_admin, N(k_admin) },
    { OS_FB_APP_API, "/api", "Tokens API", "api", k_api, N(k_api) },
    { OS_FB_APP_PM, "/pm", "Outils PM", "promptmessage", k_pm, N(k_pm) },
    { OS_FB_APP_PROC, "/proc", "Processus", "processes", k_proc, N(k_proc) },
    /* /p2p stays the desktop pane (pane=p2p). This menu is /mesh. */
    { OS_FB_APP_P2PMESH, "/mesh", "Maillage P2P", "p2p", k_p2pmesh, N(k_p2pmesh) },
    { OS_FB_APP_CHILD, "/child", "Enfants", "children", k_child, N(k_child) },
    { OS_FB_APP_RELEASE, "/release", "Livraison", "release", k_release, N(k_release) },
    { OS_FB_APP_KEYS, "/keys", "Cles collab", "keys", k_keys, N(k_keys) },
    { OS_FB_APP_CMD, "/cmd", "Commande", "any", 0, 0 },
};
const int osui_par_family_count = N(osui_par_families);

static int p_eq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

const osui_par_family_t *osui_par_family(int kind) {
    int i;
    for (i = 0; i < osui_par_family_count; i++) if (osui_par_families[i].kind == kind) return &osui_par_families[i];
    return 0;
}

int osui_par_kind_of(const char *slash) {
    int i;
    if (!slash) return 0;
    for (i = 0; i < osui_par_family_count; i++) if (p_eq(osui_par_families[i].slash, slash)) return osui_par_families[i].kind;
    return 0;
}

const char *osui_par_cmd_name(const osui_par_entry_t *e, char *out, int max) {
    int i = 0;
    while (e->cmd[i] && e->cmd[i] != ' ' && i < max - 1) { out[i] = e->cmd[i]; i++; }
    out[i] = 0;
    return out;
}

int osui_par_build(const osui_par_entry_t *e, const char *input, char *out, int max) {
    int o = 0, i;
    if ((e->flags & OSUI_PAR_ARG) && (!input || !input[0])) { if (max > 0) out[0] = 0; return -1; }
    for (i = 0; e->cmd[i] && o < max - 1; i++) out[o++] = e->cmd[i];
    if ((e->flags & OSUI_PAR_ARG) && o < max - 1) {
        out[o++] = ' ';
        for (i = 0; input[i] && o < max - 1; i++) out[o++] = input[i];
    }
    out[o] = 0;
    return 0;
}

int osui_path_is_image(const char *p) {
    int n = 0;
    char a, b, c;
    if (!p) return 0;
    while (p[n]) n++;
    if (n <= 4 || p[n - 4] != '.') return 0;
    a = p[n - 3]; b = p[n - 2]; c = p[n - 1];
    if (a == 'p' && (b == 'g' || b == 'p' || b == 'n') && c == 'm') return 1;
    if (a == 'b' && b == 'm' && c == 'p') return 1;
    if (a == 'p' && b == 'n' && c == 'g') return 1;
    if (a == 'j' && b == 'p' && c == 'g') return 1;
    return 0;
}

int osui_par_slash_list(void (*put)(const char *s)) {
    int i, n = 0;
    if (!put) return 0;
    for (i = 0; i < osui_par_builtin_count; i++, n++) {
        put(osui_par_builtins[i].slash); put("  "); put(osui_par_builtins[i].what); put("\n");
    }
    for (i = 0; i < osui_par_family_count; i++, n++) {
        put(osui_par_families[i].slash); put("  "); put(osui_par_families[i].title); put("\n");
    }
    put("ai TEXTE  question au modele; tout autre mot: commande du chat\n");
    return n;
}
