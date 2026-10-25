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
    { "Chercher dans un repertoire (recursif)", "grep -r", A, "MOTIF [REPERTOIRE]" },
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
