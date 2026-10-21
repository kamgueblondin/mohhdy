# Plan du Lot 21 - Mohhdy OS

## Objectif
Perfectionnement de l'Orchestration des PromptPrograms (.pm), Consolidation du Moteur P2P Multi-Sauts & Systeme de Fichiers Virtuel (US-038 / US-069 / US-081).

## Tableau des Taches et Statuts

| Tache | Description | Statut |
|---|---|---|
| 1. Orchestration PromptPrograms (.pm) & Isolation Runtime | Perfectionner la compilation et l'execution sandboxee des PromptPrograms (.pm / PMC1) avec verification des signatures Schnorr (.sig / pmsig1) via csig_verify, verification de la chaine de confiance CA local (/etc/pm_ca.pub) et revocations (CRL / pm_crl_has) traçees dans osui-audit (US-038 / US-045). | Termine |
| 2. Maillage P2P Multi-Sauts, gRPC-Wire & Auto-Healing | Optimiser le routage mesh multi-sauts (p2p_route_find) avec detection/evitement des boucles a 2 nœuds, mise a jour du cout de lien EWMA, appels gRPC-Wire typés (p2p_grpc_call_deadline) et traversement NAT/STUN avec repli automatique (p2p_nat_punch, p2p_stun_probe) (US-069 / US-074). | Termine |
| 3. VFS Sandbox, Overlay State & Integrite de Persistance | Consolider la persistance chiffree (ENC_V1:) des 5 overlays VFS (node, prod, collab, p2pkv, web) et leur synchronisation (browser-fs-sync, browser-fs-save, browser-fs-load), la tracabilite VFS (/browser, /fs) et la verification FNV-1a (prod-support-bundle) (US-081 / US-115). | Termine |
| 4. Documentation & Tests de non-regression | Validation de la suite complete (make test-all, benchmarks GGUF KVM, extract_guest_commands.py --check, tests QEMU OS-UI runtime et GUI). | Termine |
