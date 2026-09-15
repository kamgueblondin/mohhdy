# AOS-1577…1584 — Latence GGUF : cache de curseur FAT16 et kernels quantifiés

**Statut : livré et mesuré.** Ce lot diminue le coût de lecture du runtime GPT-2 GGUF local sans charger les poids en mémoire, sans allocation dynamique et sans modifier l’ABI du shell. Il traite le chevauchement physique entre deux lignes quantifiées successives et réduit les opérations arithmétiques répétées dans les kernels Q4_K et Q6_K.

> **Constat initial.** Une ligne quantifiée de 768 canaux occupe 324, 432 ou 630 octets selon Q3_K, Q4_K ou Q6_K. Deux lectures de lignes successives peuvent donc partager un secteur FAT16 de 512 octets ; sans cache, ce secteur était relu au périphérique.

## Livraison

| Composant | Modification | Garantie |
|---|---|---|
| `fat16_file_t` | Cache d’un secteur de 512 octets et LBA associé, possédés par le curseur | L’état est caller-owned ; aucun cache global, heap ou allocation implicite n’est ajouté. |
| `fat16_file_read` | Réutilisation du secteur caché lorsque le LBA demandé est identique | La sémantique des lectures partielles et des limites de fichier est préservée. |
| `fat16_file_seek` | Invalidation explicite du cache lors du repositionnement | Un saut ne peut pas réutiliser un secteur associé à une position antérieure. |
| Kernel Q4_K | Facteurs d’échelle et minima pré-calculés par segment de 64 valeurs | L’ordre des opérations utiles et le résultat des vecteurs Unity sont conservés. |
| Kernel Q6_K | Facteurs `d × scale` pré-calculés par segment de 128 valeurs | Les produits de la tête de logits et des matrices Q6_K évitent des multiplications répétées. |
| Smoke QEMU GGUF | Publication du temps mur entre l’envoi de `ai bonjour` et le marqueur de réponse locale | La régression peut être mesurée par la cible versionnée `make qemu-gguf-smoke`. |

## Contrat mémoire

Le cache ajoute exactement **512 octets** et quelques métadonnées au curseur FAT16. Chaque lecteur de tenseur quantifié continue d’ouvrir un curseur local, de se positionner sur le tenseur, puis de consommer ses lignes séquentiellement. Les poids du modèle restent exclusivement sur le volume FAT16.

| Ressource | Avant | Après | Allocation dynamique |
|---|---:|---:|---|
| Catalogue GGUF | 2 MiB statiques | 2 MiB statiques | Aucune |
| Workspace et cache KV | inchangés | inchangés | Aucune |
| Curseur FAT16 | métadonnées de position | métadonnées + 512 octets | Aucune |
| Poids GGUF | FAT16 par fenêtres | FAT16 par fenêtres | Aucune |

## Mesure QEMU TCG

La mesure utilise le disque de déploiement Q3_K réel, sélectionne `gpt2.gguf`, soumet `ai bonjour` et attend le marqueur `[GPT-2 GGUF local]`. Les runs QEMU TCG présentent une variabilité limitée liée à l’émulation ; la référence initiale a été **19,13 s** et le run avec cache de curseur a observé **17,03 s**, soit un gain indicatif d’environ **11 %**. Le run combinant aussi le pré-calcul des échelles a mesuré **17,32 s**, ce qui reste dans la variabilité d’émulation mais confirme l’absence de régression fonctionnelle.

| Commande | Validation |
|---|---|
| `make test-all` | La suite inclut le cache FAT16 et les kernels quantifiés. |
| `make qemu-gguf-smoke` | Affiche la durée du premier token local sur le modèle réel. |
| `make qemu-smoke` | Conserve les scénarios système standard. |

## Vecteurs ajoutés

Le test `test_cursor_caches_shared_sector` compte les lectures de secteur de la fixture. Après une première fenêtre de trois octets du fichier `FATOK.TXT`, une seconde fenêtre de deux octets appartenant au même secteur ne déclenche pas de lecture périphérique supplémentaire. Les vecteurs Q4_K et Q6_K existants continuent de valider les produits quantifiés après le pré-calcul des facteurs.

## Limites et suite

Le cache est volontairement limité à un seul secteur par curseur : il cible le chevauchement certain des lectures séquentielles sans déplacer le coût mémoire vers un cache de modèle non borné. La latence QEMU reste dominée par le forward complet et la tête de 50 257 logits. La suite peut étudier un cache de pages statique explicitement dimensionné, des kernels vectorisés i386 SSE2 et une génération coopérative, tout en préservant le caractère sans heap du runtime.
