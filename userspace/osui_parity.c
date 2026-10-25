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
};
static const osui_par_entry_t k_store[] = {
    { "Pilote ATA (lectures, ecritures)", "ata-status", 0, 0 },
    { "Points de montage", "mount-journal", 0, 0 },
    { "Persistance (blobs)", "persist-status", 0, 0 },
    { "Lister un repertoire", "ls", A, "CHEMIN (ex: /samples)" },
    { "Infos d'un fichier", "stat", A, "CHEMIN" },
    { "Creer un repertoire", "mkdir", A | M, "CHEMIN" },
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
    { "Verifier le deploiement", "deploy-verify", 0, 0 },
    { "Integrite d'un repertoire", "prod-integrity", A, "baseline|check DIR" },
};
static const osui_par_entry_t k_security[] = {
    { "Identite (whoami)", "whoami", 0, 0 },
    { "Cles d'identite", "id-key", 0, 0 },
    { "Origine (politique)", "origin-check", 0, 0 },
    { "Verifier une signature CA", "pm-ca-verify", A, "FICHIER" },
    { "Initialiser la CA locale", "pm-ca-init", M, 0 },
    { "Signer un fichier", "pm-ca-sign", A | M, "FICHIER" },
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
};
static const osui_par_entry_t k_notes[] = {
    { "Lister la racine (notes de l'overlay)", "ls /", 0, 0 },
    { "Lire une note", "cat", A, "FICHIER (ex: /notes.txt)" },
    { "Ajouter une ligne a une note", "append", A | M, "FICHIER TEXTE (ex: /notes.txt)" },
    { "Archiver un repertoire", "migrate-export", A | M, "DIR ARCHIVE" },
    { "Verifier une archive", "migrate-verify", A, "ARCHIVE [DIR]" },
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

const osui_par_family_t osui_par_families[] = {
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
