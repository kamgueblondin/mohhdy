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
#define MAX_SYSCALLS 127

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

#define OS_SOCKET_BAD_ARGUMENT (-120)
#define OS_SOCKET_NO_SLOT (-121)
#define OS_SOCKET_NOT_OPEN (-122)
#define OS_SOCKET_NOT_CONNECTED (-123)
#define OS_SOCKET_BUFFER_SMALL (-124)
#define OS_SOCKET_PROTOCOL (-125)

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

/* Événement local de supervision : child_pid est l’enfant concerné ; related_pid
 * désigne le superviseur entrant ou sortant lors d’une délégation ; detail vaut
 * le motif de sortie pour OS_TASK_SUPERVISION_EXIT et zéro sinon. */
typedef struct {
    uint32_t sequence;
    uint32_t action;
    int32_t child_pid;
    int32_t related_pid;
    uint32_t detail;
    uint32_t ticks;
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
} os_task_metrics_t;

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
    if (event_out->sequence == 0U || event_out->child_pid <= 0 ||
        event_out->action < OS_TASK_SUPERVISION_EXIT ||
        event_out->action > OS_TASK_SUPERVISION_DELEGATE_IN) return -1;
    if (((event_out->action == OS_TASK_SUPERVISION_DELEGATE_OUT ||
          event_out->action == OS_TASK_SUPERVISION_DELEGATE_IN) && event_out->related_pid <= 0) ||
        ((event_out->action != OS_TASK_SUPERVISION_DELEGATE_OUT &&
          event_out->action != OS_TASK_SUPERVISION_DELEGATE_IN) && event_out->related_pid != 0)) return -1;
    return 0;
}

#endif
