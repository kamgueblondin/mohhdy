/* os_syscalls.h - ABI Ring 3 / noyau (numéros et structures POD uniquement) */

#ifndef OS_SYSCALLS_H
#define OS_SYSCALLS_H

#include <stdint.h>

#define SYS_EXIT     0
#define SYS_PUTC     1
#define SYS_GETC     2
#define SYS_PUTS     3
#define SYS_YIELD    4
#define SYS_GETS     5
#define SYS_EXEC     6
#define SYS_SPAWN    7
#define SYS_LISTDIR  8
#define SYS_READFILE 9
#define SYS_GETPID   10
#define SYS_PS       11
#define SYS_KILL     12
#define SYS_TICKS    13
#define SYS_MEMINFO  14
#define SYS_MKDIR    15
#define SYS_UNLINK   16
#define SYS_WRITEFILE 17
#define SYS_STAT     18
#define SYS_RENAME   19
#define SYS_COPY     20
#define SYS_APPEND   21
/* prompt (EBX), buffer de reponse (ECX), taille du buffer (EDX) */
#define SYS_GPT2_GENERATE 22
/* EBX = PID cible, ECX = os_ipc_payload_t* */
#define SYS_IPC_SEND      23
/* EBX = os_ipc_message_t* */
#define SYS_IPC_RECV      24
/* EBX = nom de service ; le PID est celui de l’appelant Ring 3. */
#define SYS_SERVICE_REGISTER 25
/* EBX = nom de service ; EAX reçoit le PID associé. */
#define SYS_SERVICE_LOOKUP   26
/* EBX = nom de service ; seul son propriétaire peut le retirer. */
#define SYS_SERVICE_UNREGISTER 27
/* EBX = nom de service, ECX = PID bénéficiaire ; transfert par le propriétaire. */
#define SYS_SERVICE_GRANT 28
/* EBX = chemin, ECX = buffer, EDX = taille ; réservé au propriétaire de `vfs`. */
#define SYS_VFS_BACKEND_READ 29
/* EBX = nom ; abonne l’appelant Ring 3 aux changements de propriétaire. */
#define SYS_SERVICE_NOTIFY 30
/* EBX = chemin relatif, ECX = données, EDX = taille ; réservé au propriétaire de `vfs`. */
#define SYS_VFS_BACKEND_WRITE 31
/* EBX = chemin initrd relatif, ECX = buffer, EDX = taille ; réservé au propriétaire de `vfs`. */
#define SYS_VFS_INITRD_READ 32
/* EBX = chemin overlay relatif, ECX = buffer, EDX = taille ; réservé au propriétaire de `vfs`. */
#define SYS_VFS_OVERLAY_READ 33
/* EBX = chemin overlay relatif ; réservé au propriétaire de `vfs`. */
#define SYS_VFS_OVERLAY_UNLINK 34
/* EBX = ancien chemin overlay relatif, ECX = nouveau chemin relatif ; réservé au propriétaire de `vfs`. */
#define SYS_VFS_OVERLAY_RENAME 35
/* EBX = nom de service, ECX = os_service_status_t* ; état public borné. */
#define SYS_SERVICE_STATUS 36
/* EBX = chemin initrd relatif, ECX = os_dirent_t* ; réservé au propriétaire de `vfs`. */
#define SYS_VFS_INITRD_STAT 37
/* EBX = chemin overlay relatif, ECX = os_dirent_t* ; réservé au propriétaire de `vfs`. */
#define SYS_VFS_OVERLAY_STAT 38
/* EBX = chemin initrd relatif, ECX = os_dirent_t*, EDX = max_n ; réservé au propriétaire de `vfs`. */
#define SYS_VFS_INITRD_LISTDIR 39
/* EBX = chemin overlay relatif, ECX = os_dirent_t*, EDX = max_n ; réservé au propriétaire de `vfs`. */
#define SYS_VFS_OVERLAY_LISTDIR 40
/* EBX = chemin initrd relatif, ECX = os_dirent_t*, EDX = index logique ; page de cinq entrées. */
#define SYS_VFS_INITRD_LISTDIR_PAGE 41
/* EBX = chemin overlay relatif, ECX = os_dirent_t*, EDX = index logique ; page de cinq entrées. */
#define SYS_VFS_OVERLAY_LISTDIR_PAGE 42
/* EBX = chemin overlay relatif ; réservé au propriétaire de `vfs`. */
#define SYS_VFS_OVERLAY_MKDIR 43
/* EBX = chemin overlay relatif ; réservé au propriétaire de `vfs`. */
#define SYS_VFS_OVERLAY_RMDIR 44
/* EBX = nom de service, ECX = PID bénéficiaire ; capacité backend déléguée par le propriétaire. */
#define SYS_SERVICE_BACKEND_GRANT 45
/* EBX = nom de service, ECX = PID bénéficiaire ; révocation par le propriétaire. */
#define SYS_SERVICE_BACKEND_REVOKE 46
/* EBX = nom de service, ECX = PID bénéficiaire, EDX = masque de droits backend. */
#define SYS_SERVICE_BACKEND_GRANT_SCOPED 47
/* EBX = nom de service, ECX = PID bénéficiaire, EDX = uint32_t* ; réservé au propriétaire. */
#define SYS_SERVICE_BACKEND_STATUS       48
/* EBX = nom de service, ECX = os_service_backend_list_t* ; réservé au propriétaire. */
#define SYS_SERVICE_BACKEND_LIST         49
/* EBX = nom, ECX = génération attendue, EDX = os_service_backend_snapshot_t* ; réservé au propriétaire. */
#define SYS_SERVICE_BACKEND_OBSERVE      50
/* EBX = PID cible, ECX = os_task_metrics_t* ; instantané de télémétrie locale. */
#define SYS_TASK_METRICS                 51
/* EBX = PID cible, ECX = priorité [1,3] ; politique CPU locale. */
#define SYS_TASK_SET_PRIORITY            52
/* EBX = PID enfant ; bloque le parent jusqu’au départ de son enfant direct. */
#define SYS_TASK_WAIT                    53
/* EBX = PID cible, ECX = nom NUL-termine ; autorité locale soi/enfant direct. */
#define SYS_TASK_SET_NAME                54
/* EBX = os_task_capacity_t* ; instantané global de capacité des tâches. */
#define SYS_TASK_CAPACITY                55
/* EBX = PID enfant, ECX = os_task_exit_result_t* ; dernier résultat local du parent. */
#define SYS_TASK_CHILD_RESULT            56
/* EBX = os_task_exit_history_t* ; historique borné du parent appelant. */
#define SYS_TASK_CHILD_RESULT_LIST       57
/* Aucun argument ; efface l’historique local et retourne sa nouvelle génération. */
#define SYS_TASK_CHILD_RESULT_ACK        58
/* EBX = génération attendue, ECX = os_task_exit_history_observation_t*. */
#define SYS_TASK_CHILD_RESULT_OBSERVE    59
/* EBX = PID enfant, ECX = os_task_exit_result_t* ; recherche dans l’historique local. */
#define SYS_TASK_CHILD_RESULT_FIND       60
/* EBX = PID enfant ; retire une entrée locale et retourne la nouvelle génération. */
#define SYS_TASK_CHILD_RESULT_FORGET     61
/* EBX = PID enfant direct ; suspend une tâche prête sans la terminer. */
#define SYS_TASK_SUSPEND                 62
/* EBX = PID enfant direct suspendu ; le rend à nouveau planifiable. */
#define SYS_TASK_RESUME                  63
/* Aucun argument ; termine l’instantané des enfants directs et retourne leur nombre. */
#define SYS_TASK_KILL_CHILDREN           64
/* EBX = os_task_children_t* ; instantané borné des enfants directs actifs. */
#define SYS_TASK_CHILDREN                65
/* Aucun argument ; bloque le parent jusqu’au départ d’un enfant direct. */
#define SYS_TASK_WAIT_ANY                66
/* EBX = os_task_child_exit_count_t* ; compteur cumulatif local de départs directs. */
#define SYS_TASK_CHILD_EXIT_COUNT        67
/* EBX = PID enfant direct, ECX = PID nouveau superviseur utilisateur. */
#define SYS_TASK_DELEGATE_CHILD          68
/* EBX = os_task_supervision_events_t* ; journal borné du parent courant. */
#define SYS_TASK_SUPERVISION_EVENTS      69
/* Aucun argument ; acquitte le journal local et retourne sa nouvelle génération. */
#define SYS_TASK_SUPERVISION_EVENTS_ACK  70
/* EBX = génération attendue, ECX = os_task_supervision_events_observation_t*. */
#define SYS_TASK_SUPERVISION_EVENTS_OBSERVE 71
/* EBX = séquence, ECX = os_task_supervision_event_t*. */
#define SYS_TASK_SUPERVISION_EVENT_FIND 72
/* EBX = séquence ; oublie l’entrée retenue et retourne le nombre restant. */
#define SYS_TASK_SUPERVISION_EVENT_FORGET 73
/* EBX = os_task_supervision_summary_t* ; instantané local consolidé. */
#define SYS_TASK_SUPERVISION_SUMMARY 74
/* EBX = 0 (désabonne) ou 1 (abonne) l’appelant à ses transitions de supervision. */
#define SYS_TASK_SUPERVISION_NOTIFY 75
/* EBX = masque local des transitions à notifier lorsque la souscription est active. */
#define SYS_TASK_SUPERVISION_NOTIFY_FILTER 76
/* EBX = os_task_supervision_notify_status_t* ; instantané local de souscription. */
#define SYS_TASK_SUPERVISION_NOTIFY_STATUS 77
/* EBX = PID enfant (0 avec ECX=0 pour désactiver/vider) ; ECX = 0 retire ou 1 ajoute. */
#define SYS_TASK_SUPERVISION_WATCH 78
/* EBX = os_task_supervision_watch_status_t* ; instantané local de la watchlist. */
#define SYS_TASK_SUPERVISION_WATCH_STATUS 79
/* EBX = os_task_supervision_delivery_stats_t* ; compteurs locaux de livraison détaillée. */
#define SYS_TASK_SUPERVISION_DELIVERY_STATS 80
/* Aucun argument ; remet les compteurs locaux de livraison détaillée à zéro. */
#define SYS_TASK_SUPERVISION_DELIVERY_STATS_ACK 81
/* EBX = séquence locale ; rediffuse best-effort l’événement détaillé retenu. */
#define SYS_TASK_SUPERVISION_EVENT_REPLAY 82
/* EBX = PID enfant direct ; zéro efface la sélection prioritaire locale. */
#define SYS_TASK_SUPERVISION_PRIORITY 83
/* EBX = os_task_supervision_priority_status_t* ; sélection prioritaire locale. */
#define SYS_TASK_SUPERVISION_PRIORITY_STATUS 84

/* EBX = budget de tentatives détaillées ; zéro désactive la limite locale. */
#define SYS_TASK_SUPERVISION_NOTIFY_BUDGET 85
/* EBX = os_task_supervision_notify_budget_status_t* ; état du budget local. */
#define SYS_TASK_SUPERVISION_NOTIFY_BUDGET_STATUS 86
/* EBX = chemin 8.3, ECX = buffer, EDX = taille maximale ; lecture FAT16. */
#define SYS_FAT16_READ 87
/* EBX = tableau os_dirent_t, ECX = capacité ; liste de la racine FAT16. */
#define SYS_FAT16_LIST 88
/* Aucun argument; bit 0 = NIC détectée, bit 1 = anneaux initialisés. */
#define SYS_NET_STATUS 89
/* Aucun argument; bit 0 = NE2000 prêt, bit 1 = bail DHCP, bit 2 = entropie RDRAND, bit 3 = ancre X.509, bits 8..15 = phase LLM. */
#define SYS_LLM_SESSION_STATUS 90
/* EBX = os_llm_acquire_start_request_t* ; démarre DHCP→DNS→SYN sans secret. */
#define SYS_LLM_ACQUIRE_START 91
/* Aucun argument ; pilote SYN-ACK/TLS avec seuls les buffers persistants du noyau. */
#define SYS_LLM_POLL_TLS 92
/* EBX = os_llm_request_t* ; émet un POST LLM seulement après TLS authentifié. */
#define SYS_LLM_REQUEST 93
/* EBX = os_llm_text_result_t* ; copie seulement le texte fournisseur extrait. */
#define SYS_LLM_POLL_TEXT 94
/* EBX = os_llm_text_result_t* ; copie un delta SSE, sans buffer interne. */
#define SYS_LLM_POLL_SSE 95
/* Aucun argument ; réarme RESPONSE_READY vers TLS_COMPLETE pour le tour suivant. */
#define SYS_LLM_RESET_FOR_REQUEST 96
/* Aucun argument ; tente un FIN TCP si la session est établie puis purge localement la session. */
#define SYS_LLM_CLOSE 97
/* EBX = os_llm_openai_credential_request_t* ; copie un bearer borné dans le noyau. */
#define SYS_LLM_OPENAI_CREDENTIAL 98
/* EBX = port local, ECX = port distant, EDX = séquence locale. */
#define SYS_SOCKET_OPEN 99
/* EBX = descripteur, ECX = os_socket_syn_ack_t* caller-owned. */
#define SYS_SOCKET_ACCEPT_SYN_ACK 100
/* EBX = os_socket_send_request_t* caller-owned. */
#define SYS_SOCKET_SEND 101
/* EBX = os_socket_feed_request_t* caller-owned. */
#define SYS_SOCKET_FEED 102
/* EBX = os_socket_receive_request_t* caller-owned. */
#define SYS_SOCKET_RECEIVE 103
/* EBX = descripteur socket. */
#define SYS_SOCKET_CLOSE 104
/* EBX = port local, ECX = séquence initiale. */
#define SYS_SOCKET_LISTEN 105
/* EBX = descripteur, ECX = os_socket_passive_view_t* avec SYN entrant. */
#define SYS_SOCKET_ACCEPT_SYN 106
/* EBX = descripteur, ECX = buffer SYN-ACK, EDX = capacité, ESI = out_length. */
#define SYS_SOCKET_BUILD_SYN_ACK 107
/* EBX = descripteur, ECX = os_socket_passive_view_t* avec ACK final. */
#define SYS_SOCKET_ACCEPT_ACK 108
/* prompt (EBX), buffer de réponse (ECX), taille du buffer (EDX), backend GGUF FAT16. */
#define SYS_GPT2_GGUF_GENERATE 109
/* buffer de réponse (ECX), taille du buffer (EDX) ; poursuit la session GGUF locale. */
#define SYS_GPT2_GGUF_CONTINUE 110
/* EBX = chemin FAT32 8.3, ECX = buffer, EDX = taille maximale ; lecture esclave. */
#define SYS_FAT32_READ 111
/* EBX = tableau os_dirent_t, ECX = capacité ; liste de la racine FAT32 esclave. */
#define SYS_FAT32_LIST 112
/* EBX = tableau os_dirent_t, ECX = capacité, EDX = départ ; page de racine FAT16. */
#define SYS_FAT16_LIST_PAGE 113
/* EBX = tableau os_dirent_t, ECX = capacité, EDX = départ ; page de racine FAT32. */
#define SYS_FAT32_LIST_PAGE 114
/* EBX = nom de service ; le bénéficiaire courant abandonne sa capacité backend. */
#define SYS_SERVICE_BACKEND_RELEASE 115
/* EBX = chemin FAT16 : racine 8.3/LFN ou DIR/8.3, ECX = données (nul+0 pour mkdir), EDX = taille ; réservé à `vfs` mutate. */
#define SYS_VFS_FAT16_CREATE 116
/* EBX = chemin FAT16 : racine 8.3/LFN, DIR/8.3 ou DIR/ pour rmdir ; réservé à `vfs` mutate. */
#define SYS_VFS_FAT16_UNLINK 117
/* EBX = ancien chemin FAT16, ECX = nouveau chemin ; racine 8.3/LFN ou même DIR/8.3, réservé à `vfs` mutate. */
#define SYS_VFS_FAT16_RENAME 118
/* EBX = chemin FAT32 : racine 8.3/LFN ou DIR/8.3, ECX = données (nul+0 pour mkdir), EDX = taille ; réservé à `vfs` mutate. */
#define SYS_VFS_FAT32_CREATE 119
/* EBX = chemin FAT32 : racine 8.3/LFN, DIR/8.3 ou DIR/ pour rmdir ; réservé à `vfs` mutate. */
#define SYS_VFS_FAT32_UNLINK 120
/* EBX = ancien chemin FAT32, ECX = nouveau chemin ; racine 8.3/LFN ou même DIR/8.3, réservé à `vfs` mutate. */
#define SYS_VFS_FAT32_RENAME 121
/* EBX = chemin de répertoire FAT16, ECX = os_dirent_t*, EDX = capacité, ESI = départ ; lecture Ring 3 comme SYS_FAT16_READ. */
#define SYS_FAT16_LIST_PATH 122
/* EBX = chemin de répertoire FAT32, ECX = os_dirent_t*, EDX = capacité, ESI = départ ; lecture Ring 3 comme SYS_FAT32_READ. */
#define SYS_FAT32_LIST_PATH 123
/* EBX = service, ECX = PID, EDX = droits, ESI = sources ; réservé au propriétaire. */
#define SYS_SERVICE_BACKEND_GRANT_SCOPED_SOURCE 124
/* EBX = service, ECX = PID, EDX = os_service_backend_scope_t* ; réservé au propriétaire. */
#define SYS_SERVICE_BACKEND_SCOPE_STATUS 125
/* EBX = service, ECX = PID, EDX = droits, ESI = sources, EDI = préfixe relatif NUL-termine ; réservé au propriétaire. */
#define SYS_SERVICE_BACKEND_GRANT_SCOPED_SOURCE_PREFIX 126
/* EBX = os_fb_scene_t* (magic OS_FB_MAGIC) pour le bureau VBE ; EBX = 0 pour quitter. */
#define SYS_VGA_BLIT 127
/* EBX = os_peer_listen_request_t* : ecoute passive guest puis SYN-ACK NE2000. */
#define SYS_PEER_LISTEN 128
/* EBX = os_peer_accept_request_t* : poll SYN/ACK jusqu a SYN_RECEIVED ou ESTABLISHED. */
#define SYS_PEER_ACCEPT 129
/* EBX = os_peer_tls_poll_request_t* : role serveur TLS guest (ServerHello..Finished). */
#define SYS_PEER_TLS_POLL 130
/* Tranche 4 slice 2 (Ring 3 ATA driver). 131-134 are reserved to the live
 * ata-driver owner (OS_ATA_DRIVER_REQUIRED otherwise); 135 is a public
 * read-only snapshot. */
/* Take the controller: opens the ATA ports in the TSS IOPB for the caller. */
#define SYS_ATA_CLAIM 131
/* Give the controller back: ports closed again, kernel PIO allowed. */
#define SYS_ATA_RELEASE 132
/* EBX = os_ata_job_t*, ECX = buffer of OS_ATA_JOB_MAX_SECTORS*512 bytes.
 * Returns 1 and fills the job (and the data for a write) or 0 if idle. */
#define SYS_ATA_JOB_FETCH 133
/* EBX = const os_ata_job_t*, ECX = driver status, EDX = buffer (read data). */
#define SYS_ATA_JOB_DONE 134
/* EBX = os_ata_status_t* : counters and client write fences. */
#define SYS_ATA_STATUS 135
/* Tranche 4 test hook (root shell only): EBX = OS_ATA_DEBUG_CRASH_FAT_WRITE
 * arms a one-shot crash of the driver in the middle of its next FAT write
 * job, to prove the Ring 0 fallback after a driver death mid-transfer. */
#define SYS_ATA_DEBUG 136
/* Tranche 5 slice 2 (net IPC relay). 137 is reserved to the live net-driver
 * worker: EBX = const os_net_relay_reply_t*, answers the relayed request
 * the kernel forwarded to it over IPC (OS_IPC_NET_RELAY_REQUEST). */
#define SYS_NET_RELAY_REPLY 137
/* Public read-only: EBX = os_net_relay_status_t*. */
#define SYS_NET_RELAY_STATUS 138
/* Tranche 5 slice 3 (wire TCP through the worker). 139-142 are reserved to
 * the live net-driver worker PID (OS_NET_WORKER_REQUIRED otherwise); they
 * drive real NE2000 frames for a socket of the Ring 0 registry. */
/* EBX = os_net_wire_connect_t* : ARP-resolve, emit SYN, complete handshake. */
#define SYS_NET_WIRE_CONNECT 139
/* EBX = os_net_wire_io_t* : build a data segment, emit it, poll until the
 * peer ACKs it (payload that arrives meanwhile is queued in the socket). */
#define SYS_NET_WIRE_SEND 140
/* EBX = os_net_wire_io_t* : poll frames, demux to the bound socket, ACK. */
#define SYS_NET_WIRE_RECV 141
/* EBX = socket id : emit FIN and drop the wire binding. */
#define SYS_NET_WIRE_CLOSE 142
/* Public read-only: EBX = os_net_wire_status_t*. */
#define SYS_NET_WIRE_STATUS 143
/* EBX = os_socket_connect_request_t* : open + wire-connect in one relayed
 * call (public, gated and relayed like the other socket syscalls). */
#define SYS_SOCKET_CONNECT 144
/* Tranche 4 suite (FAT/overlay logic out of Ring 0 at runtime). EBX selects
 * an OS_ATA_FS_* sub-operation; all of them but OS_ATA_FS_STATUS are
 * reserved to the live ata-driver (OS_ATA_DRIVER_REQUIRED otherwise). The
 * driver runs its own FAT16/FAT32/overlay code on its own PIO; the kernel
 * only marshals bytes (see docs/tranche4_ata_ring3_driver.md, "suite"). */
#define SYS_ATA_FS 145
/* Tranche 5 suite (NE2000 driver out of Ring 0). EBX selects an
 * OS_NET_NIC_* sub-operation; CLAIM/PUMP/IRQ are reserved to the live
 * net-driver worker (OS_NET_WORKER_REQUIRED otherwise), STATUS is public.
 * After CLAIM the worker owns the NE2000 ports 0x300-0x31F through the TSS
 * I/O bitmap and gets IRQ3 as a counter; the kernel no longer touches the
 * NIC until the worker dies (reclaim). See docs/tranche5_net_worker_gate.md. */
#define SYS_NET_NIC 146
/* Tranche 5 pile: bulk side channel of the net relay (LLM structs larger
 * than one IPC payload). EBX = OS_NET_RELAY_BULK_* , ECX = job id, EDX =
 * buffer, ESI = length / capacity. Live net-driver PID only (-59). */
#define SYS_NET_RELAY_BULK 147
/* EBX = nom du service, ECX = uint32_t* : jeton non nul de la capacite
 * backend detenue par l'appelant. 0 en memoire n'est pas une capacite. */
#define SYS_SERVICE_BACKEND_TOKEN 148
/* EBX = os_service_event_pull_t* : plus vieil evenement non acquitte de
 * l'appelant, puis acquitte. Independant de la boite IPC. */
#define SYS_SERVICE_EVENT_PULL 149
/* EBX = 1 ajout (ECX = prefixe, EDX = source) ou 2 liste (ECX = tampon,
 * EDX = capacite). Journal disque hors zone AIOV et hors BPB FAT. */
#define SYS_MOUNT_JOURNAL 150
/* EBX = uint32_t* : cle d'identite de la tache appelante. Jamais nulle. */
#define SYS_TASK_IDENTITY_KEY 151
/* EBX = os_ipc_spill_drops_t* : pertes service et supervision, compteurs separes. */
#define SYS_IPC_SPILL_DROPS 152
/* EBX = nom, ECX = uint32_t* : right_token du nom dont l'appelant est le titulaire. */
#define SYS_SERVICE_RIGHT_TOKEN 153
#define MAX_SYSCALLS 154
#define OS_ATA_DEBUG_CRASH_FAT_WRITE 1U

#define OS_VGA_COLS 80
#define OS_VGA_ROWS 25
#define OS_VGA_KEY_ESC 27
#define OS_VGA_KEY_LEFT 28
#define OS_VGA_KEY_RIGHT 29
#define OS_VGA_KEY_UP 30
#define OS_VGA_KEY_DOWN 31

#define OS_FB_MAGIC 0x4F534642u /* 'OSFB' */
#define OS_FB_CHAT_CENTER 0
#define OS_FB_CHAT_FLOAT 1
#define OS_FB_PANE_NONE 0
#define OS_FB_PANE_BROWSER 1
#define OS_FB_PANE_SHELL 2
#define OS_FB_PANE_ADMIN 3
#define OS_FB_PANE_SUPPORT 4
#define OS_FB_PANE_STATUS 5
#define OS_FB_PANE_FS 6
#define OS_FB_STAGE_REFLECTING 0
#define OS_FB_STAGE_ACTING 1
#define OS_FB_STAGE_PRESENTING 2
#define OS_FB_KIND_PLAN 0
#define OS_FB_KIND_CIRCLE 1
#define OS_FB_KIND_BOXES 2
#define OS_FB_KIND_GRAPH 3
#define OS_FB_KIND_TREE 4
#define OS_FB_KIND_CLOCK 5
#define OS_FB_KIND_SIM 6
#define OS_FB_MSG_MAX 6
#define OS_FB_MSG_LEN 120
#define OS_FB_INPUT_LEN 96
#define OS_FB_TERM_MAX 14
#define OS_FB_TERM_LEN 64

typedef struct {
    uint16_t cells[OS_VGA_ROWS * OS_VGA_COLS];
} os_vga_frame_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint8_t chat_mode;
    uint8_t pane;
    uint8_t stage_mode;
    uint8_t stage_kind;
    uint16_t chat_x;
    uint16_t chat_y;
    uint16_t nmsg;
    uint8_t nterm;
    uint8_t reserved;
    char session[12];
    char input[OS_FB_INPUT_LEN];
    char messages[OS_FB_MSG_MAX][OS_FB_MSG_LEN];
    char term[OS_FB_TERM_MAX][OS_FB_TERM_LEN];
} os_fb_scene_t;

typedef struct {
    uint16_t source_port;
    uint16_t destination_port;
    uint32_t sequence;
    uint32_t acknowledgment;
    uint8_t flags;
} os_socket_syn_ack_t;
typedef struct {
    int32_t socket_id;
    const uint8_t* payload;
    uint16_t length;
    uint8_t* segment;
    uint16_t capacity;
    uint16_t* out_length;
} os_socket_send_request_t;
typedef struct {
    int32_t socket_id;
    const uint8_t* segment;
    uint16_t length;
} os_socket_feed_request_t;
typedef struct {
    int32_t socket_id;
    uint8_t* buffer;
    uint16_t capacity;
    uint16_t* out_length;
} os_socket_receive_request_t;
typedef struct {
    uint16_t source_port;
    uint16_t destination_port;
    uint32_t sequence;
    uint32_t acknowledgment;
    uint8_t flags;
} os_socket_passive_view_t;

typedef struct {
    uint16_t local_port;
    uint32_t local_sequence;
} os_peer_listen_request_t;

typedef struct {
    uint16_t attempts;
    uint8_t require_established;
} os_peer_accept_request_t;

typedef struct {
    uint16_t attempts;
    uint8_t metier; /* 1 : echange applicatif METIER, pas le poll handshake */
} os_peer_tls_poll_request_t;

#define OS_PEER_BAD_REQUEST (-130)
#define OS_PEER_UNAVAILABLE (-131)
#define OS_PEER_NO_LEASE (-132)
#define OS_PEER_IN_PROGRESS (-133)
#define OS_PEER_FAILED (-134)
#define OS_PEER_TIMEOUT (-135)
#define OS_PEER_NOT_LISTENING (-136)


#define OS_SOCKET_BAD_ARGUMENT (-120)
#define OS_SOCKET_NO_SLOT (-121)
#define OS_SOCKET_NOT_OPEN (-122)
#define OS_SOCKET_NOT_CONNECTED (-123)
#define OS_SOCKET_BUFFER_SMALL (-124)
#define OS_SOCKET_PROTOCOL (-125)
/* Tranche 5: NE2000 / socket / LLM-network / peer syscall from a task that is
 * not the live net-driver worker PID while that worker is registered. -60 is
 * kept free for OS_VFS_BACKEND_WORKER_REQUIRED (AOS-2177, PR #62). */
#define OS_NET_WORKER_REQUIRED (-59)
/* Tranche 5 slice 2: a relayed socket call got no worker reply in time. */
#define OS_NET_RELAY_TIMEOUT (-87)
/* Tranche 5 slice 2: net-driver died after taking the request; outcome is
 * unknown, the kernel does not replay it. */
#define OS_NET_RELAY_ABORTED (-88)
/* Tranche 5 slice 3: the wire handshake / echo did not complete in time. */
#define OS_NET_WIRE_TIMEOUT (-126)
/* Tranche 5 slice 3: no NE2000 present, so no wire path. */
#define OS_NET_WIRE_UNAVAILABLE (-127)
/* Tranche 5 slice 3: socket is not wire-bound; the worker falls back to the
 * in-registry (segment codec) path of slice 2. */
#define OS_NET_WIRE_NOT_BOUND (-119)

/* Requête POD sans pointeur : hostname, ports et budgets uniquement. */
#define OS_LLM_HOSTNAME_MAX 96U
#define OS_LLM_ACQUIRE_MAX_ATTEMPTS 8U
#define OS_LLM_MODEL_MAX 64U
#define OS_LLM_PATH_MAX 64U
#define OS_LLM_PROMPT_MAX 1024U
#define OS_LLM_TEXT_MAX 2048U
#define OS_LLM_BEARER_MAX 128U
typedef struct {
    char hostname[OS_LLM_HOSTNAME_MAX];
    uint32_t xid;
    uint32_t local_sequence;
    uint16_t dns_id;
    uint16_t dhcp_attempts;
    uint16_t dns_attempts;
    uint16_t arp_attempts;
    uint16_t local_port;
    uint16_t remote_port;
} os_llm_acquire_start_request_t;

/* Requête utilisateur bornée sans identifiant fournisseur, clé ni pointeur. */
typedef struct {
    uint8_t provider;
    uint8_t streaming;
    char model[OS_LLM_MODEL_MAX];
    char path[OS_LLM_PATH_MAX];
    uint16_t prompt_length;
    uint8_t prompt[OS_LLM_PROMPT_MAX];
} os_llm_request_t;

/* Credential OpenAI POD : le token n’est jamais retourné par un syscall. */
typedef struct { char bearer[OS_LLM_BEARER_MAX]; } os_llm_openai_credential_request_t;

/* Sortie copiée par valeur : texte extrait et code HTTP, jamais un buffer TLS interne. */
typedef struct {
    uint16_t text_length;
    uint16_t status_code;
    uint8_t text[OS_LLM_TEXT_MAX];
} os_llm_text_result_t;

#define OS_LLM_ACQUIRE_BAD_REQUEST (-90)
#define OS_LLM_ACQUIRE_UNAVAILABLE (-91)
#define OS_LLM_ACQUIRE_IN_PROGRESS (-92)
#define OS_LLM_ACQUIRE_FAILED (-93)
#define OS_LLM_ACQUIRE_TLS_ENTROPY (-112)
#define OS_LLM_ACQUIRE_DHCP_FAILED (-113)
#define OS_LLM_ACQUIRE_BOOTSTRAP_FAILED (-114)
#define OS_LLM_ACQUIRE_DHCP_DISCOVER_FAILED (-115)
#define OS_LLM_ACQUIRE_DHCP_OFFER_TIMEOUT (-116)
#define OS_LLM_ACQUIRE_DHCP_REQUEST_FAILED (-117)
#define OS_LLM_ACQUIRE_DHCP_ACK_TIMEOUT (-118)
#define OS_LLM_TLS_BAD_PHASE (-94)
#define OS_LLM_TLS_UNCONFIGURED (-95)
#define OS_LLM_TLS_FAILED (-96)
#define OS_LLM_REQUEST_BAD_REQUEST (-97)
#define OS_LLM_REQUEST_BAD_PHASE (-98)
#define OS_LLM_REQUEST_UNCONFIGURED (-99)
#define OS_LLM_REQUEST_FAILED (-100)
#define OS_LLM_CREDENTIAL_BAD_ARGUMENT (-109)
#define OS_LLM_CREDENTIAL_BAD_PHASE (-110)
#define OS_LLM_TEXT_BAD_ARGUMENT (-101)
#define OS_LLM_TEXT_BAD_PHASE (-102)
#define OS_LLM_TEXT_FAILED (-103)
#define OS_LLM_SSE_BAD_ARGUMENT (-104)
#define OS_LLM_SSE_BAD_PHASE (-105)
#define OS_LLM_SSE_FAILED (-106)
#define OS_LLM_RESET_BAD_PHASE (-107)
#define OS_LLM_RESET_FAILED (-108)
#define OS_LLM_TLS_ENTROPY_UNAVAILABLE (-109)
#define OS_LLM_CLOSE_BAD_PHASE (-110)
/* Le FIN n’a pas été émis, mais la purge locale de session et de secrets a réussi. */
#define OS_LLM_CLOSE_FIN_FAILED (-111)

/* IPC Foundation : messages courts, copies par valeur et retours non bloquants. */
#define OS_IPC_MAX_DATA 96U
#define OS_IPC_EMPTY       (-40)
#define OS_IPC_FULL        (-41)
#define OS_IPC_BAD_TARGET  (-42)
#define OS_IPC_BAD_MESSAGE (-43)
/* L’endpoint d’un propriétaire de service est saturé par la politique de
 * service avant la capacité brute de la tâche. */
#define OS_IPC_SERVICE_FULL (-44)

/* Registre Foundation : simple découverte de nom, pas une capability. */
#define OS_SERVICE_NAME_MAX 16U
#define OS_SERVICE_BACKEND_CAPACITY 4U
/* Préfixe relatif d’une source ; la chaîne vide représente la racine entière. */
#define OS_SERVICE_BACKEND_PREFIX_MAX 48U
#define OS_SERVICE_BAD_NAME  (-50)
#define OS_SERVICE_FULL      (-51)
#define OS_SERVICE_TAKEN     (-52)
#define OS_SERVICE_NOT_FOUND (-53)
#define OS_SERVICE_NOT_OWNER (-54)
#define OS_SERVICE_BAD_GRANTEE (-55)
#define OS_SERVICE_WATCH_FULL  (-56)
#define OS_SERVICE_STALE        (-57)
#define OS_VFS_BACKEND_DENIED (-61)
/* Tranche 4: ata-driver service / ATA sector IPC reserved to the dedicated
 * atadriver binary or refused because no Ring 3 ATA driver is live. -59 and
 * -60 are kept for the net / VFS worker codes of PR #64 / #62. */
#define OS_ATA_DRIVER_REQUIRED (-58)
/* AOS-2177: historical SYS_READFILE/SYS_WRITEFILE hit the ATA-backed overlay
 * while vfs-virtual is live and the caller is not the worker PID. */
#define OS_VFS_BACKEND_WORKER_REQUIRED (-60)
/* Sources de backend pour le scope de lecture, sous forme de bitmask. */
#define OS_SERVICE_BACKEND_SOURCE_INITRD  (1U << 0)
#define OS_SERVICE_BACKEND_SOURCE_OVERLAY (1U << 1)
#define OS_SERVICE_BACKEND_SOURCE_FAT16   (1U << 2)
#define OS_SERVICE_BACKEND_SOURCE_FAT32   (1U << 3)
#define OS_SERVICE_BACKEND_SOURCE_ALL     (OS_SERVICE_BACKEND_SOURCE_INITRD | \
                                           OS_SERVICE_BACKEND_SOURCE_OVERLAY | \
                                           OS_SERVICE_BACKEND_SOURCE_FAT16 | \
                                           OS_SERVICE_BACKEND_SOURCE_FAT32)
#define OS_TASK_NOT_FOUND    (-62)
#define OS_TASK_BAD_PRIORITY    (-63)
/* Le demandeur n’est ni la tâche cible ni son parent direct. */
#define OS_TASK_CONTROL_DENIED (-64)
/* La tâche cible n’est pas un enfant direct du demandeur. */
#define OS_TASK_NOT_CHILD      (-65)
/* Le parent a atteint sa capacité locale d’enfants directs. */
#define OS_TASK_CHILD_LIMIT    (-66)
/* Le nouveau nom est vide, trop long ou contient un caractère non imprimable. */
#define OS_TASK_BAD_NAME       (-67)
/* La file bornée de tâches actives est pleine. */
#define OS_TASK_GLOBAL_LIMIT   (-68)
/* Le parent ne conserve aucun dernier résultat pour cet enfant. */
#define OS_TASK_NO_CHILD_RESULT (-69)
/* La génération attendue de l’historique enfant ne correspond plus. */
#define OS_TASK_HISTORY_STALE (-70)
/* La tâche cible ne peut pas effectuer la transition de cycle de vie demandée. */
#define OS_TASK_BAD_STATE (-71)
/* Le parent appelant ne possède aucun enfant direct actif à superviser. */
#define OS_TASK_NO_DIRECT_CHILD (-72)
/* Le nouveau superviseur créerait une filiation invalide ou cyclique. */
#define OS_TASK_BAD_DELEGATE (-73)
/* Aucune transition de supervision retenue ne porte cette séquence locale. */
#define OS_TASK_NO_SUPERVISION_EVENT (-74)
/* La valeur de souscription de supervision doit être strictement 0 ou 1. */
#define OS_TASK_BAD_NOTIFY (-75)
/* Le masque de notifications contient un bit inconnu. */
#define OS_TASK_BAD_NOTIFY_FILTER (-76)
/* La commande de watchlist ou son argument d’activation est invalide. */
#define OS_TASK_BAD_WATCH (-77)
/* La watchlist locale de supervision a atteint sa capacité bornée. */
#define OS_TASK_WATCH_FULL (-78)
/* Le PID demandé n’est pas retenu dans la watchlist locale. */
#define OS_TASK_NO_SUPERVISION_WATCH (-79)
#define OS_FAT16_NOT_MOUNTED    (-80)
#define OS_FAT16_BAD_PATH       (-81)
#define OS_FAT16_NOT_FOUND      (-82)
#define OS_FAT16_CORRUPT        (-83)
#define OS_FAT16_BUFFER_SMALL   (-84)

#define OS_TASK_EXIT_KILLED (-128)
#define OS_TASK_EXIT_HISTORY_CAPACITY 4U

#define OS_NAME_MAX 64
#define OS_PROC_NAME_MAX 32
#define OS_TASK_GLOBAL_CAPACITY 16U

#define OS_DIRENT_FILE 0
#define OS_DIRENT_DIR  1

#define OS_TASK_RUNNING   0
#define OS_TASK_READY     1
#define OS_TASK_WAITING   2
#define OS_TASK_SUSPENDED 3
#define OS_TASK_TERMINATED 4

#define OS_TASK_KERNEL 0
#define OS_TASK_USER   1

#define OS_TASK_PRIORITY_LOW     1U
#define OS_TASK_PRIORITY_NORMAL  2U
#define OS_TASK_PRIORITY_HIGH    3U
#define OS_TASK_CHILD_CAPACITY   4U
#define OS_TASK_SUPERVISION_EVENT_CAPACITY 4U
#define OS_TASK_SUPERVISION_WATCH_CAPACITY OS_TASK_CHILD_CAPACITY

#define OS_TASK_SUPERVISION_EXIT          1U
#define OS_TASK_SUPERVISION_SUSPEND       2U
#define OS_TASK_SUPERVISION_RESUME        3U
#define OS_TASK_SUPERVISION_DELEGATE_OUT  4U
#define OS_TASK_SUPERVISION_DELEGATE_IN   5U

#define OS_TASK_SUPERVISION_NOTIFY_EXIT         (1U << 0)
#define OS_TASK_SUPERVISION_NOTIFY_SUSPEND      (1U << 1)
#define OS_TASK_SUPERVISION_NOTIFY_RESUME       (1U << 2)
#define OS_TASK_SUPERVISION_NOTIFY_DELEGATE_OUT (1U << 3)
#define OS_TASK_SUPERVISION_NOTIFY_DELEGATE_IN  (1U << 4)
#define OS_TASK_SUPERVISION_NOTIFY_ALL          (OS_TASK_SUPERVISION_NOTIFY_EXIT | \
                                                  OS_TASK_SUPERVISION_NOTIFY_SUSPEND | \
                                                  OS_TASK_SUPERVISION_NOTIFY_RESUME | \
                                                  OS_TASK_SUPERVISION_NOTIFY_DELEGATE_OUT | \
                                                  OS_TASK_SUPERVISION_NOTIFY_DELEGATE_IN)

typedef struct {
    char name[OS_NAME_MAX];
    uint32_t size;
    uint32_t flags; /* OS_DIRENT_FILE / OS_DIRENT_DIR */
} os_dirent_t;

/* Entrée FAT16 8.3 normalisée vers le format public de listage. */
typedef struct {
    char name[OS_NAME_MAX];
    uint32_t size;
    uint32_t flags;
} os_fat16_dirent_t;

typedef struct {
    int32_t pid;
    uint32_t rights;
} os_service_backend_entry_t;

/* Statut interne borné de capacité backend. Le détail des sources n’est pas
 * exposé dans les réponses IPC publiques de vfs-backend-status. */
typedef struct {
    uint32_t rights;
    uint32_t sources;
} os_service_backend_scope_t;

/* Retrait d'un evenement de service hors de la boite IPC. */
typedef struct {
    char name[OS_SERVICE_NAME_MAX];
    int32_t old_pid;
    int32_t new_pid;
    uint32_t reason;
    uint32_t sequence;
} os_service_event_pull_t;

typedef struct {
    uint32_t count;
    os_service_backend_entry_t entries[OS_SERVICE_BACKEND_CAPACITY];
} os_service_backend_list_t;

typedef struct {
    uint32_t generation;
    os_service_backend_list_t list;
} os_service_backend_snapshot_t;

typedef struct {
    int32_t pid;
    int32_t parent_pid; /* -1 lorsqu’aucun parent utilisateur n’est connu. */
    int32_t state;
    int32_t type;
    char name[OS_PROC_NAME_MAX];
    /* sequence, generation, identity_key : seulement si identity_visible vaut 1
     * (l'appelant est la tache, ou son parent direct). Sinon les trois restent a 0. */
    uint32_t sequence;
    uint32_t generation;
    uint32_t identity_key;
    uint32_t identity_visible;
} os_proc_t;

/* Instantané local et non atomique des enfants directs actifs d’un parent. */
typedef struct {
    uint32_t count;
    os_proc_t entries[OS_TASK_CHILD_CAPACITY];
} os_task_children_t;

/* Total cumulatif local de départs d’enfants directs depuis la création du parent. */
typedef struct {
    uint32_t count;
} os_task_child_exit_count_t;

/* Evenement local de supervision : child_pid est l'enfant concerne ; related_pid
 * designe le superviseur entrant ou sortant lors d'une delegation ; detail vaut
 * le motif de sortie pour OS_TASK_SUPERVISION_EXIT et zero sinon.
 * child_sequence, child_generation et identity_key sont les temoins de l'enfant
 * copies a l'enregistrement. Ils restent dans le journal RAM du parent et
 * meurent avec lui. L'avis IPC reste sur 24 octets et ne les porte pas. */
typedef struct {
    uint32_t sequence;
    uint32_t action;
    int32_t child_pid;
    int32_t related_pid;
    uint32_t detail;
    uint32_t ticks;
    uint32_t child_sequence;
    uint32_t child_generation;
    uint32_t identity_key;
} os_task_supervision_event_t;

/* Instantané circulaire local, de l’événement le plus ancien au plus récent. */
typedef struct {
    uint32_t generation;
    uint32_t count;
    os_task_supervision_event_t entries[OS_TASK_SUPERVISION_EVENT_CAPACITY];
} os_task_supervision_events_t;

/* Lecture conditionnelle : la génération est toujours renseignée, même si
 * l’appel retourne OS_TASK_HISTORY_STALE. */
typedef struct {
    uint32_t generation;
    os_task_supervision_events_t events;
} os_task_supervision_events_observation_t;

/* Agrégat local, non atomique : les champs peuvent refléter des instants
 * différents si la supervision change pendant leur collecte. */
typedef struct {
    uint32_t generation;
    uint32_t active_children;
    uint32_t suspended_children;
    uint32_t child_exit_count;
    uint32_t retained_events;
} os_task_supervision_summary_t;

/* Instantané local de la souscription IPC de supervision. enabled vaut 0 ou 1 ;
 * mask contient les bits OS_TASK_SUPERVISION_NOTIFY_* demandés. */
typedef struct {
    uint32_t enabled;
    uint32_t mask;
} os_task_supervision_notify_status_t;

/* Watchlist locale de notifications détaillées. Si enabled vaut zéro, tous les
 * enfants directs restent admissibles ; s’il vaut un, seuls les PID retenus le sont. */
typedef struct {
    uint32_t enabled;
    uint32_t count;
    int32_t pids[OS_TASK_SUPERVISION_WATCH_CAPACITY];
} os_task_supervision_watch_status_t;

/* Compteurs locaux, volatils et non atomiques. attempted ne progresse qu’après
 * souscription, filtre d’action et watchlist ; dropped signifie saturation IPC. */
typedef struct {
    uint32_t attempted;
    uint32_t delivered;
    uint32_t dropped;
} os_task_supervision_delivery_stats_t;

/* Un seul enfant direct peut être prioritaire ; child_pid vaut -1 lorsqu’aucun
 * enfant n’est sélectionné. */
typedef struct {
    int32_t child_pid;
} os_task_supervision_priority_status_t;

typedef struct {
    uint32_t limit;
    uint32_t used;
} os_task_supervision_notify_budget_status_t;

/* Instantané local, non atomique : le temps exécuté est compté en ticks
 * d’horloge entre deux commutations de tâches. */
typedef struct {
    int32_t pid;
    int32_t parent_pid; /* -1 lorsqu’aucun parent utilisateur n’est connu. */
    int32_t state;
    int32_t type;
    uint32_t priority;
    uint32_t created_ticks;
    uint32_t age_ticks;
    uint32_t run_ticks;
    uint32_t switch_count;
    uint32_t direct_children;
    uint32_t sequence;
    uint32_t generation;
    uint32_t identity_key;
    uint32_t identity_visible;
} os_task_metrics_t;

/* Pertes du deversoir RAM. service : evenements de service. supervision :
 * notifications de sortie, suspension, reprise et delegation. Chacun sature
 * a 0xFFFFFFFF. */
typedef struct {
    uint32_t service;
    uint32_t supervision;
} os_ipc_spill_drops_t;

/* Instantané global de la file de tâches ; il est volatil et non atomique. */
typedef struct {
    uint32_t active;
    uint32_t capacity;
    uint32_t available;
} os_task_capacity_t;

/* Dernier résultat d’enfant retenu localement par son parent. Non atomique,
 * non persistant et remplacé par le départ direct suivant. */
typedef struct {
    int32_t child_pid;
    int32_t exit_code;
    uint32_t reason;
    uint32_t finished_ticks;
} os_task_exit_result_t;

/* Historique circulaire local, ramené dans l’ordre du plus ancien au plus récent. */
typedef struct {
    uint32_t count;
    os_task_exit_result_t entries[OS_TASK_EXIT_HISTORY_CAPACITY];
} os_task_exit_history_t;

/* Observation optimiste de l’historique enfant local. La génération ne réserve rien. */
typedef struct {
    uint32_t generation;
    os_task_exit_history_t history;
} os_task_exit_history_observation_t;

/* Instantané local d’un endpoint propriétaire de service. Il n’est ni
 * atomique, ni réservé, ni une capability. */
typedef struct {
    int32_t owner_pid;
    uint32_t queued_messages;
    uint32_t client_capacity;
    uint32_t endpoint_capacity;
} os_service_status_t;

typedef struct {
    uint32_t total_pages;
    uint32_t used_pages;
    uint32_t free_pages;
} os_meminfo_t;

/* Charge fournie par l'émetteur : son identité est ajoutée par le noyau.
 * request_id est opaque et permet au protocole utilisateur de corréler une réponse.
 */
typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t request_id;
    uint8_t data[OS_IPC_MAX_DATA];
} os_ipc_payload_t;

/* Message délivré au destinataire depuis sa boîte aux lettres noyau. */
typedef struct {
    int32_t sender_pid;
    uint32_t type;
    uint32_t size;
    uint32_t request_id;
    uint8_t data[OS_IPC_MAX_DATA];
} os_ipc_message_t;

/* Notification synthétique, émise par le noyau (sender_pid = 0) dans l’IPC
 * existant. La charge est : nom NUL-paddé, ancien PID, nouveau PID, raison. */
#define OS_IPC_SERVICE_EVENT 0x53525601U
#define OS_SERVICE_EVENT_SIZE (OS_SERVICE_NAME_MAX + 12U)
#define OS_SERVICE_EVENT_PUBLISHED    1U
#define OS_SERVICE_EVENT_GRANTED      2U
#define OS_SERVICE_EVENT_UNREGISTERED 3U
#define OS_SERVICE_EVENT_PURGED       4U

/* Notification noyau best-effort vers le parent direct lors de la sortie d’un enfant. */
#define OS_IPC_TASK_EVENT 0x54415301U
#define OS_TASK_EVENT_SIZE 8U
#define OS_TASK_EVENT_EXITED 1U
#define OS_TASK_EVENT_KILLED 2U

typedef struct {
    char name[OS_SERVICE_NAME_MAX];
    int32_t old_owner_pid;
    int32_t new_owner_pid;
    uint32_t reason;
} os_service_event_t;

static inline void os_service_encode_i32(uint8_t* out, int32_t value) {
    uint32_t raw = (uint32_t)value;
    out[0] = (uint8_t)(raw & 0xffU);
    out[1] = (uint8_t)((raw >> 8) & 0xffU);
    out[2] = (uint8_t)((raw >> 16) & 0xffU);
    out[3] = (uint8_t)((raw >> 24) & 0xffU);
}

static inline int32_t os_service_decode_i32(const uint8_t* in) {
    uint32_t raw = (uint32_t)in[0] | ((uint32_t)in[1] << 8) |
                   ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
    return (int32_t)raw;
}

static inline void os_ipc_encode_u32(uint8_t* out, uint32_t value) {
    out[0] = (uint8_t)(value & 0xffU);
    out[1] = (uint8_t)((value >> 8) & 0xffU);
    out[2] = (uint8_t)((value >> 16) & 0xffU);
    out[3] = (uint8_t)((value >> 24) & 0xffU);
}

static inline uint32_t os_ipc_decode_u32(const uint8_t* in) {
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) |
           ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
}

static inline int os_service_make_event(os_ipc_payload_t* payload, const char* name,
                                        int32_t old_owner_pid, int32_t new_owner_pid,
                                        uint32_t reason) {
    uint32_t i;
    int terminated = 0;
    if (!payload || !name || old_owner_pid < 0 || new_owner_pid < 0 ||
        reason < OS_SERVICE_EVENT_PUBLISHED || reason > OS_SERVICE_EVENT_PURGED) return -1;
    for (i = 0U; i < OS_SERVICE_NAME_MAX; i++) {
        payload->data[i] = (uint8_t)name[i];
        if (name[i] == '\0') {
            terminated = 1;
            for (i++; i < OS_SERVICE_NAME_MAX; i++) payload->data[i] = 0U;
            break;
        }
    }
    if (!terminated) return -1;
    payload->type = OS_IPC_SERVICE_EVENT;
    payload->size = OS_SERVICE_EVENT_SIZE;
    payload->request_id = 0U;
    os_service_encode_i32(&payload->data[OS_SERVICE_NAME_MAX], old_owner_pid);
    os_service_encode_i32(&payload->data[OS_SERVICE_NAME_MAX + 4U], new_owner_pid);
    payload->data[OS_SERVICE_NAME_MAX + 8U] = (uint8_t)(reason & 0xffU);
    payload->data[OS_SERVICE_NAME_MAX + 9U] = (uint8_t)((reason >> 8) & 0xffU);
    payload->data[OS_SERVICE_NAME_MAX + 10U] = (uint8_t)((reason >> 16) & 0xffU);
    payload->data[OS_SERVICE_NAME_MAX + 11U] = (uint8_t)((reason >> 24) & 0xffU);
    for (i = OS_SERVICE_EVENT_SIZE; i < OS_IPC_MAX_DATA; i++) payload->data[i] = 0U;
    return 0;
}

static inline int os_service_parse_event(const os_ipc_message_t* message,
                                         os_service_event_t* event_out) {
    uint32_t i;
    uint32_t reason;
    if (!message || !event_out || message->sender_pid != 0 ||
        message->type != OS_IPC_SERVICE_EVENT || message->size != OS_SERVICE_EVENT_SIZE ||
        message->request_id != 0U) return -1;
    for (i = 0U; i < OS_SERVICE_NAME_MAX; i++) event_out->name[i] = (char)message->data[i];
    if (event_out->name[OS_SERVICE_NAME_MAX - 1U] != '\0') return -1;
    event_out->old_owner_pid = os_service_decode_i32(&message->data[OS_SERVICE_NAME_MAX]);
    event_out->new_owner_pid = os_service_decode_i32(&message->data[OS_SERVICE_NAME_MAX + 4U]);
    reason = (uint32_t)message->data[OS_SERVICE_NAME_MAX + 8U] |
             ((uint32_t)message->data[OS_SERVICE_NAME_MAX + 9U] << 8) |
             ((uint32_t)message->data[OS_SERVICE_NAME_MAX + 10U] << 16) |
             ((uint32_t)message->data[OS_SERVICE_NAME_MAX + 11U] << 24);
    if (event_out->old_owner_pid < 0 || event_out->new_owner_pid < 0 ||
        reason < OS_SERVICE_EVENT_PUBLISHED || reason > OS_SERVICE_EVENT_PURGED) return -1;
    event_out->reason = reason;
    return 0;
}

typedef struct {
    int32_t child_pid;
    uint32_t reason;
} os_task_event_t;

static inline int os_task_make_event(os_ipc_payload_t* payload, int32_t child_pid,
                                     uint32_t reason) {
    if (!payload || child_pid <= 0 ||
        (reason != OS_TASK_EVENT_EXITED && reason != OS_TASK_EVENT_KILLED)) return -1;
    payload->type = OS_IPC_TASK_EVENT;
    payload->size = OS_TASK_EVENT_SIZE;
    payload->request_id = 0U;
    os_service_encode_i32(payload->data, child_pid);
    payload->data[4] = (uint8_t)(reason & 0xffU);
    payload->data[5] = (uint8_t)((reason >> 8) & 0xffU);
    payload->data[6] = (uint8_t)((reason >> 16) & 0xffU);
    payload->data[7] = (uint8_t)((reason >> 24) & 0xffU);
    return 0;
}

static inline int os_task_parse_event(const os_ipc_message_t* message,
                                      os_task_event_t* event_out) {
    uint32_t reason;
    if (!message || !event_out || message->sender_pid != 0 ||
        message->type != OS_IPC_TASK_EVENT || message->size != OS_TASK_EVENT_SIZE ||
        message->request_id != 0U) return -1;
    event_out->child_pid = os_service_decode_i32(message->data);
    reason = (uint32_t)message->data[4] | ((uint32_t)message->data[5] << 8) |
             ((uint32_t)message->data[6] << 16) | ((uint32_t)message->data[7] << 24);
    if (event_out->child_pid <= 0 ||
        (reason != OS_TASK_EVENT_EXITED && reason != OS_TASK_EVENT_KILLED)) return -1;
    event_out->reason = reason;
    return 0;
}

/* Notification noyau best-effort de toute transition retenue lorsqu’un parent
 * a explicitement activé sa souscription locale. */
#define OS_IPC_TASK_SUPERVISION_EVENT 0x54415302U

/* Tranche 4: Ring 3 ATA driver IPC. The driver owns the ATA port capability
 * (TSS IOPB) and serves bounded windows of one sector (payload <= 96 bytes, no
 * shared memory yet). Only the ATA sector client service owner may ask.
 * data layout: [0..3] lba, [4] drive, [5] offset/16 (0..31), [6] len (<=64),
 * [8..71] bytes for WRITE. Reply: [0..3] status, [4..67] bytes for READ. */
#define OS_IPC_ATA_READ   0x41544101U
#define OS_IPC_ATA_WRITE  0x41544102U
#define OS_IPC_ATA_REPLY  0x41544103U
#define OS_ATA_IPC_WINDOW 64U
#define OS_ATA_IPC_CLIENT_SERVICE "ata-client"
/* LBA 0-63 of the master disk hold the kernel overlay snapshot (Ring 0). */
#define OS_ATA_KERNEL_RESERVED_LBAS 64U

/* Tranche 4 slice 2: kernel <-> atadriver multi-sector job protocol. The
 * kernel copies up to 8 sectors per chunk between its overlay snapshot buffer
 * and the driver buffer (no shared page); the driver runs the PIO at CPL 3. */
#define OS_ATA_OVERLAY_SECTORS 64U
#define OS_ATA_JOB_MAX_SECTORS 8U
#define OS_ATA_JOB_NONE  0U
#define OS_ATA_JOB_WRITE 1U
#define OS_ATA_JOB_READ  2U
/* Tranche 4 slice 3: synchronous FAT sector job (drive 0 or 1). */
#define OS_ATA_JOB_IO_READ  3U
#define OS_ATA_JOB_IO_WRITE 4U
/* SYS_ATA_JOB_DONE results (>= 0). */
#define OS_ATA_JOB_CHUNK_OK      0
#define OS_ATA_JOB_FLUSH_DONE    1
#define OS_ATA_JOB_LOAD_DONE     2
#define OS_ATA_JOB_LOAD_SKIPPED  3
#define OS_ATA_JOB_FAILED        4
#define OS_ATA_JOB_IO_DONE       5
/* Kernel PIO refused because the Ring 3 driver holds the controller. */
#define OS_ATA_CONTROLLER_BUSY (-85)
/* Job completion does not match the chunk currently handed out. */
#define OS_ATA_JOB_STALE (-86)

typedef struct {
    uint32_t op;
    uint32_t drive;
    uint32_t lba;
    uint32_t count;
    uint32_t generation;
    uint32_t flags;            /* OS_ATA_JOB_FLAG_* */
} os_ata_job_t;

/* Tranche 4 suite: filesystem operation served by the driver's own
 * FAT16/FAT32/overlay code. SYS_ATA_JOB_FETCH hands out a job with
 * op = OS_ATA_JOB_FS_OP (count 0, no sector data); the driver copies the
 * request with OS_ATA_FS_REQUEST and completes it with OS_ATA_FS_DONE. */
#define OS_ATA_JOB_FS_OP 7U
#define OS_ATA_JOB_FS_DONE 6 /* OS_ATA_FS_DONE accepted */

/* SYS_ATA_FS sub-operations (EBX). */
#define OS_ATA_FS_READY       1U /* ECX=store flags, EDX=image buffer, ESI=capacity */
#define OS_ATA_FS_REQUEST     2U /* ECX=os_ata_job_t*, EDX=buffer, ESI=capacity */
#define OS_ATA_FS_NOTE        3U /* ECX=generation, EDX=OS_ATA_FS_NOTE_* */
#define OS_ATA_FS_PUBLISH     4U /* ECX=generation, EDX=image, ESI=size, EDI=op result */
#define OS_ATA_FS_DONE        5U /* ECX=os_ata_job_t*, EDX=reply buffer, ESI=capacity */
#define OS_ATA_FS_INITRD_STAT 6U /* ECX=path -> OS_ATA_FS_INITRD_* bits */
#define OS_ATA_FS_INITRD_READ 7U /* ECX=path, EDX=buffer, ESI=max -> bytes */
#define OS_ATA_FS_STATUS      8U /* public: ECX=os_ata_status_t* (same as SYS_ATA_STATUS) */

/* Store flags: what the driver serves from its own code. */
#define OS_ATA_FS_STORE_FAT16   1U
#define OS_ATA_FS_STORE_FAT32   2U
#define OS_ATA_FS_STORE_OVERLAY 4U
#define OS_ATA_FS_STORE_ALL     7U

/* OS_ATA_FS_NOTE kinds. */
#define OS_ATA_FS_NOTE_SECTOR_WRITTEN 1U /* first completed sector write of the op */
#define OS_ATA_FS_NOTE_PERSISTED      2U /* overlay snapshot written to LBA 0-63 */
#define OS_ATA_FS_NOTE_PERSIST_FAILED 3U

#define OS_ATA_FS_INITRD_FILE 1
#define OS_ATA_FS_INITRD_DIR  2

/* Request/reply buffers (kernel <-> driver copies, no shared page). */
#define OS_ATA_FSOP_BUFFER_SIZE 8192U
#define OS_ATA_FSOP_PATH_MAX 256U
#define OS_ATA_FSOP_MAGIC 0x504F5346U /* 'FSOP' */

/* Operations. FAT32 codes are the FAT16 ones + OS_ATA_FSOP_FAT32_BASE. */
#define OS_ATA_FSOP_FAT16_READ      1U  /* path, arg0=max            -> bytes */
#define OS_ATA_FSOP_FAT16_LIST      2U  /* arg0=capacity             -> dirents */
#define OS_ATA_FSOP_FAT16_LIST_PAGE 3U  /* arg0=capacity, arg1=start -> dirents */
#define OS_ATA_FSOP_FAT16_LIST_PATH 4U  /* path, arg0=cap, arg1=start-> dirents */
#define OS_ATA_FSOP_FAT16_CREATE    5U  /* path, in=data, arg0=1 for mkdir */
#define OS_ATA_FSOP_FAT16_UNLINK    6U  /* path */
#define OS_ATA_FSOP_FAT16_RENAME    7U  /* path, path2 */
#define OS_ATA_FSOP_FAT32_BASE      8U
#define OS_ATA_FSOP_OVL_READ        17U /* path, arg0=max -> bytes */
#define OS_ATA_FSOP_OVL_WRITE       18U /* path, in=data */
#define OS_ATA_FSOP_OVL_APPEND      19U /* path, in=data */
#define OS_ATA_FSOP_OVL_MKDIR       20U /* path */
#define OS_ATA_FSOP_OVL_UNLINK      21U /* path */
#define OS_ATA_FSOP_OVL_RENAME      22U /* path, path2 */
#define OS_ATA_FSOP_OVL_COPY        23U /* path, path2 */
#define OS_ATA_FSOP_OVL_STAT        24U /* path -> one os_dirent_t */
#define OS_ATA_FSOP_OVL_LISTDIR     25U /* path, in=out[0..arg0), arg0=start, arg1=max_n -> out[0..max_n) */
#define OS_ATA_FSOP_OVL_LISTDIR_PAGE 26U /* path, arg0=start, arg1=max_n -> dirents */
#define OS_ATA_FSOP_OVL_IS_DIR      27U /* path */

typedef struct {
    uint32_t magic;     /* OS_ATA_FSOP_MAGIC */
    uint32_t op;        /* OS_ATA_FSOP_* */
    uint32_t arg0;
    uint32_t arg1;
    uint32_t path_len;  /* bytes of path (NUL excluded), 0 if none */
    uint32_t path2_len;
    uint32_t in_len;    /* input bytes after the two NUL-terminated paths */
    uint32_t out_cap;   /* output bytes the caller can take */
} os_ata_fsop_request_t;

typedef struct {
    int32_t result;          /* return value of the operation */
    uint32_t out_len;        /* output bytes following this header */
    uint32_t sectors_read;   /* sectors the driver read for this op */
    uint32_t sectors_written;
} os_ata_fsop_reply_t;

/* ------------------------------------------------------------------------
 * Tranche 5 suite: NE2000 owned by the Ring 3 net-driver worker.
 * ------------------------------------------------------------------------ */
#define OS_NET_NIC_CLAIM  1U /* ECX = os_net_nic_info_t* (out) */
#define OS_NET_NIC_PUMP   2U /* ECX = os_net_nic_pump_t* */
#define OS_NET_NIC_IRQ    3U /* -> IRQ3 events since the last call */
#define OS_NET_NIC_STATUS 4U /* public: ECX = os_net_nic_status_t* */
/* Tranche 5 pile (ARP/IPv4/TCP/TLS in the Ring 3 worker). */
#define OS_NET_NIC_LOG     5U /* live net-driver: ECX = text, EDX = length, one atomic line */
#define OS_NET_NIC_UTC     6U /* owner: ECX = char[16] YYYYMMDDHHMMSSZ (TLS validity) */
#define OS_NET_NIC_PUBLISH 7U /* owner: ECX = const os_net_stack_report_t* */
#define OS_NET_NIC_STACK   8U /* public: ECX = os_net_stack_report_t* (last published) */
#define OS_NET_NIC_LOG_MAX 192U

#define OS_NET_NIC_BASE_PORT 0x300U
#define OS_NET_NIC_LAST_PORT 0x31FU
#define OS_NET_NIC_IRQ_LINE 3U
#define OS_NET_NIC_FRAME_MAX 1536U
#define OS_NET_NIC_PUMP_TX_MAX 4U

typedef struct {
    uint16_t base_port;
    uint8_t irq;
    uint8_t mac[6];       /* MAC the kernel probed at boot */
    uint8_t reserved;
} os_net_nic_info_t;

/* One engine round of the pending SYS_NET_WIRE_* / SYS_SOCKET_CONNECT op. */
#define OS_NET_NIC_PUMP_FETCH 0U /* only collect the frames queued so far */
#define OS_NET_NIC_PUMP_FRAME 1U /* rx holds a frame the worker polled */
#define OS_NET_NIC_PUMP_IDLE  2U /* nothing received this round */
typedef struct {
    uint32_t mode;                     /* OS_NET_NIC_PUMP_* */
    const uint8_t* rx;
    uint16_t rx_length;
    uint16_t tx_sent;                  /* frames of the previous pump sent OK */
    uint16_t tx_failed;
    uint16_t tx_count;                 /* out */
    uint16_t tx_length[OS_NET_NIC_PUMP_TX_MAX]; /* out */
    uint8_t* tx;                       /* OS_NET_NIC_PUMP_TX_MAX * OS_NET_NIC_FRAME_MAX */
    int32_t result;                    /* out, valid when done */
    uint32_t done;                     /* out: 1 = op finished */
} os_net_nic_pump_t;

typedef struct {
    int32_t owner_pid;       /* worker owning the ports, 0 = kernel */
    uint32_t claims;
    uint32_t reclaims;       /* kernel re-inits after a worker loss */
    uint32_t irq_forwarded;  /* IRQ3 events counted for the worker */
    uint32_t kernel_refused; /* kernel port accesses refused while owned (expected 0) */
    uint32_t kernel_gated;   /* kernel NE2000 syscalls refused while owned */
    uint32_t pumps;
    uint32_t frames_out;     /* frames handed to the worker to transmit */
    uint32_t frames_in;      /* frames the worker fed in */
    uint32_t worker_tx_ok;   /* worker-reported transmits */
    uint32_t worker_tx_failed;
} os_net_nic_status_t;

/* Kernel NE2000 path (LLM 91-98/131-138, peer 128-130, DHCP) refused: the
 * NIC is owned by the Ring 3 worker. */
#define OS_NET_NIC_WORKER_OWNED (-141)
/* SYS_NET_WIRE_* / SYS_SOCKET_CONNECT started on the worker's NIC: pump it. */
#define OS_NET_WIRE_PENDING (-142)
/* CLAIM without an NE2000 probed at boot. */
#define OS_NET_NIC_ABSENT (-143)

/* Store in Ring 3 but the driver cannot serve now (suspended, stalled): the
 * kernel does not run its own stale FS code instead (fail closed). */
#define OS_ATA_FS_UNAVAILABLE (-137)
/* Driver died after committing at least one sector of a FAT mutation:
 * outcome unknown, the kernel does not replay it. */
#define OS_ATA_FS_ABORTED (-138)
/* Driver stalled past the RPC timeout: outcome unknown, not replayed. */
#define OS_ATA_FS_TIMEOUT (-139)
/* OS_ATA_FS_READY refused while a slice 2 snapshot job is queued. */
#define OS_ATA_FS_BUSY (-140)
/* Test hook: the driver must crash in the middle of this job. */
#define OS_ATA_JOB_FLAG_DEBUG_CRASH 1U

typedef struct {
    int32_t driver_pid;        /* live ata-driver owner or 0 */
    int32_t claim_pid;         /* current controller holder or 0 */
    uint32_t flush_done;       /* overlay snapshots written by the driver */
    uint32_t load_done;        /* overlay snapshots loaded via the driver */
    uint32_t load_skipped;     /* driver loads dropped because RAM was newer */
    uint32_t job_failures;     /* chunks reported failed by the driver */
    uint32_t kernel_overlay_writes; /* overlay snapshots written by Ring 0 PIO */
    uint32_t kernel_pio_refused;    /* kernel PIO calls refused while claimed */
    uint32_t fallback_flushes;      /* Ring 0 flushes after driver loss */
    uint32_t pending;          /* 1 if a flush/load is queued or in flight */
    uint32_t client_min_lba;   /* first master LBA a client may write */
    uint32_t slave_write_locked; /* 1 if the slave disk holds FAT32 */
    /* Tranche 4 slice 3: FAT16/FAT32 sector I/O routed through the driver. */
    uint32_t fat_driver_read_sectors;  /* FAT sectors read by the driver */
    uint32_t fat_driver_write_sectors; /* FAT sectors written by the driver */
    uint32_t fat_kernel_pio_sectors;   /* FAT sectors moved by Ring 0 PIO (boot mount, fallback) */
    uint32_t fat_kernel_pio_live;      /* ... of which while a driver was live (expected 0) */
    uint32_t fat_rpc_aborts;           /* sector RPCs aborted (driver died or stalled) */
    int32_t boot_driver_pid;           /* atadriver spawned by the kernel at boot, 0 if none */
    uint32_t channel_resets;           /* ATA soft resets after a driver died holding the controller */
    uint32_t debug_crash_armed;        /* test hook pending (SYS_ATA_DEBUG) */
    /* Tranche 4 suite: FAT/overlay logic served by the driver's own code. */
    uint32_t fs_store_flags;   /* OS_ATA_FS_STORE_* currently served in Ring 3 */
    uint32_t fs_ops;           /* FS operations completed by the driver */
    uint32_t fs_kernel_live;   /* FS ops run by kernel FS code while the store was in Ring 3 (expected 0) */
    uint32_t fs_aborts;        /* FS RPCs aborted (driver died or stalled) */
    uint32_t fs_redone;        /* aborted ops redone by Ring 0 (nothing committed) */
    uint32_t fs_unavailable;   /* ops refused fail-closed (store in Ring 3, driver unable to serve) */
    uint32_t fs_publishes;     /* overlay images published into the kernel mirror */
    uint32_t fs_handovers;     /* store handovers kernel -> driver */
    uint32_t fs_restores;      /* kernel store restored from the mirror after driver loss */
} os_ata_status_t;
#define OS_TASK_SUPERVISION_EVENT_SIZE 24U

static inline int os_task_make_supervision_event(os_ipc_payload_t* payload,
                                                 const os_task_supervision_event_t* event) {
    if (!payload || !event || event->sequence == 0U || event->child_pid <= 0 ||
        event->action < OS_TASK_SUPERVISION_EXIT ||
        event->action > OS_TASK_SUPERVISION_DELEGATE_IN) return -1;
    if (((event->action == OS_TASK_SUPERVISION_DELEGATE_OUT ||
          event->action == OS_TASK_SUPERVISION_DELEGATE_IN) && event->related_pid <= 0) ||
        ((event->action != OS_TASK_SUPERVISION_DELEGATE_OUT &&
          event->action != OS_TASK_SUPERVISION_DELEGATE_IN) && event->related_pid != 0)) return -1;
    payload->type = OS_IPC_TASK_SUPERVISION_EVENT;
    payload->size = OS_TASK_SUPERVISION_EVENT_SIZE;
    payload->request_id = 0U;
    os_ipc_encode_u32(&payload->data[0], event->sequence);
    os_ipc_encode_u32(&payload->data[4], event->action);
    os_service_encode_i32(&payload->data[8], event->child_pid);
    os_service_encode_i32(&payload->data[12], event->related_pid);
    os_ipc_encode_u32(&payload->data[16], event->detail);
    os_ipc_encode_u32(&payload->data[20], event->ticks);
    return 0;
}

static inline int os_task_parse_supervision_event(const os_ipc_message_t* message,
                                                  os_task_supervision_event_t* event_out) {
    if (!message || !event_out || message->sender_pid != 0 ||
        message->type != OS_IPC_TASK_SUPERVISION_EVENT ||
        message->size != OS_TASK_SUPERVISION_EVENT_SIZE || message->request_id != 0U) return -1;
    event_out->sequence = os_ipc_decode_u32(&message->data[0]);
    event_out->action = os_ipc_decode_u32(&message->data[4]);
    event_out->child_pid = os_service_decode_i32(&message->data[8]);
    event_out->related_pid = os_service_decode_i32(&message->data[12]);
    event_out->detail = os_ipc_decode_u32(&message->data[16]);
    event_out->ticks = os_ipc_decode_u32(&message->data[20]);
    /* L'avis IPC ne porte pas les temoins. Un parse ne doit pas les laisser sales. */
    event_out->child_sequence = 0U;
    event_out->child_generation = 0U;
    event_out->identity_key = 0U;
    if (event_out->sequence == 0U || event_out->child_pid <= 0 ||
        event_out->action < OS_TASK_SUPERVISION_EXIT ||
        event_out->action > OS_TASK_SUPERVISION_DELEGATE_IN) return -1;
    if (((event_out->action == OS_TASK_SUPERVISION_DELEGATE_OUT ||
          event_out->action == OS_TASK_SUPERVISION_DELEGATE_IN) && event_out->related_pid <= 0) ||
        ((event_out->action != OS_TASK_SUPERVISION_DELEGATE_OUT &&
          event_out->action != OS_TASK_SUPERVISION_DELEGATE_IN) && event_out->related_pid != 0)) return -1;
    return 0;
}

/* Tranche 5 slice 2: net IPC relay. While net-driver is registered, the
 * socket syscalls 99-108 and peer syscalls 128-130 of any other task are
 * forwarded to the worker: the kernel sends it one IPC message (sender_pid 0,
 * type OS_IPC_NET_RELAY_REQUEST or OS_IPC_NET_PEER_RELAY_REQUEST,
 * request_id = job id) carrying the syscall number, scalar arguments and up
 * to OS_NET_RELAY_MAX_IN input bytes. The worker runs the same syscall itself
 * and answers with SYS_NET_RELAY_REPLY. */
#define OS_IPC_NET_RELAY_REQUEST      0x4E524C01U
#define OS_IPC_NET_PEER_RELAY_REQUEST 0x4E524C02U
#define OS_NET_RELAY_MAX_IN 72U
#define OS_NET_RELAY_MAX_OUT 256U
typedef struct {
    uint32_t job_id;
    uint32_t op;           /* SYS_SOCKET_* number */
    uint32_t arg0;         /* socket id or local port */
    uint32_t arg1;         /* remote port / sequence */
    uint32_t arg2;         /* sequence */
    uint16_t in_length;    /* bytes used in in[] */
    uint16_t out_capacity; /* max bytes the caller accepts back */
    uint8_t in[OS_NET_RELAY_MAX_IN];
} os_net_relay_request_t;
typedef struct {
    uint32_t job_id;
    int32_t result;
    uint32_t out_length;
    uint8_t out[OS_NET_RELAY_MAX_OUT];
} os_net_relay_reply_t;
/* Tranche 5 pile: while the worker owns the NE2000, LLM 91-98 of any
 * other task are relayed too; their structs travel through
 * SYS_NET_RELAY_BULK (request in_length = 0, arg0 = bulk bytes in,
 * out_capacity = bulk bytes expected back). */
#define OS_NET_RELAY_BULK_FETCH 1U
#define OS_NET_RELAY_BULK_PUT   2U
#define OS_NET_RELAY_BULK_MAX 2304U
typedef struct {
    uint32_t forwarded;   /* requests sent to the worker over IPC */
    uint32_t completed;   /* replies delivered back to the caller */
    uint32_t aborted;     /* worker lost with a request in flight */
    uint32_t timeouts;    /* no reply in time */
    uint32_t denied;      /* gated but not relayed (-59), e.g. LLM / peer */
    uint32_t stale;       /* replies that matched no request */
    uint32_t pending;     /* 1 while a request is in flight */
    int32_t worker_pid;   /* live net-driver PID, 0 if none */
} os_net_relay_status_t;

/* Tranche 5 slice 3: worker-only NE2000 wire path for the socket registry.
 * The worker is the only task allowed to call SYS_NET_WIRE_* (-59 otherwise);
 * the NE2000 driver, IRQ handler and TCP stack stay in Ring 0. */
#define OS_NET_WIRE_MAX_IO 256U
typedef struct {
    uint16_t local_port;
    uint16_t remote_port;
    uint8_t local_ip[4];
    uint8_t remote_ip[4];   /* on-link peer: QEMU user-net host or a guest */
    uint32_t local_sequence;
    uint16_t attempts;      /* bounded poll rounds for ARP / SYN-ACK */
} os_net_wire_connect_t;
typedef struct {
    int32_t socket_id;
    const uint8_t* data;    /* SEND: payload to emit */
    uint16_t length;        /* SEND: payload length */
    uint8_t* rx;            /* RECV: received bytes; SEND: copy of the emitted
                             * TCP segment (keeps the SYS_SOCKET_SEND contract) */
    uint16_t rx_capacity;
    uint16_t* rx_length;
    uint16_t attempts;      /* bounded poll rounds for the reply */
} os_net_wire_io_t;
typedef struct {
    uint32_t connects;      /* wire connects completed */
    uint32_t frames_tx;     /* Ethernet frames emitted on the worker path */
    uint32_t frames_rx;     /* Ethernet frames consumed on the worker path */
    uint32_t arp_tx;        /* ARP requests emitted while resolving a peer */
    uint32_t sends;         /* SYS_NET_WIRE_SEND that emitted a data segment */
    uint32_t recvs;         /* SYS_NET_WIRE_RECV that returned payload */
    uint32_t closes;        /* FINs emitted */
    uint32_t refused;       /* non-worker attempts to touch the wire (-59) */
    uint32_t demuxed;       /* rx TCP frames matched to a wire-bound socket */
    uint32_t dropped;       /* rx frames not addressed to a bound socket */
    uint32_t arp_replies;   /* ARP replies sent for a bound local IP */
    uint32_t peer_fins;     /* FINs received from wire peers */
    uint32_t bound;         /* wire-bound sockets currently tracked */
    int32_t worker_pid;     /* live net-driver PID, 0 if none */
} os_net_wire_status_t;
/* Tranche 5 pile: counters the Ring 3 stack publishes after each op. */
typedef struct {
    os_net_wire_status_t wire; /* Ring 3 ARP/IPv4/TCP wire engine */
    uint32_t llm_status;       /* SYS_LLM_SESSION_STATUS word of the Ring 3 session */
    uint32_t socket_ops;       /* relayed 99-108 run on the Ring 3 registry */
    uint32_t wire_ops;         /* relayed connect/send/recv/close on the wire */
    uint32_t llm_ops;          /* relayed 91-98 run by the Ring 3 TLS client */
    uint32_t rounds;           /* wire poll rounds run in Ring 3 */
    uint32_t frames_built;     /* frames the Ring 3 stack handed to the driver */
    uint32_t frames_parsed;    /* frames the Ring 3 stack decoded */
    uint32_t reports;          /* kernel side: accepted publications */
} os_net_stack_report_t;
/* Relayed public connect: same fields as the wire connect. */
typedef struct {
    uint16_t local_port;
    uint16_t remote_port;
    uint8_t local_ip[4];
    uint8_t remote_ip[4];
    uint32_t local_sequence;
    uint16_t attempts;
} os_socket_connect_request_t;

#endif
