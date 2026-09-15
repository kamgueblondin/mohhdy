# MOHHDY Foundation — Incrément 86 : formes 2D GPT-2 QKV et MLP

**État :** implémenté sur la branche de travail du lot 86.

## Objectif

Le lot 85 validait les invariants structurels des tenseurs quantifiés. Le lot 86 ajoute `gpt2_gguf_validate_gpt2_layer`, qui vérifie les formes consommées par le forward GPT-2 legacy, sans dépendre d’un pointeur de poids ni allouer de mémoire.

L’ordre des axes suit `gpt2_matmul_one`, qui traite la première dimension comme largeur d’entrée et la seconde comme nombre de sorties. Pour `C = channels`, le contrat est donc:

| Rôle | Forme attendue |
| --- | --- |
| normalisations et biais attention | `[C]` |
| QKV | `[C, 3C]` |
| biais QKV | `[3C]` |
| projection attention | `[C, C]` |
| biais projection attention | `[C]` |
| normalisations MLP | `[C]` |
| expansion MLP | `[C, 4C]` |
| projection MLP | `[4C, C]` |

## Garanties

La fonction exige les dix rôles présents, des canaux non nuls et multiples de `GPT2_QK_K`, puis vérifie le rang et chaque axe par rôle. Toute relation incorrecte retourne `-9`; un masque incomplet retourne `-8`. Le contrat est caller-owned et reste indépendant de la représentation physique des blocs FAT16.

## Tests

La fixture de test construit une couche synthétique avec les formes `[256]`, `[256,768]`, `[256,1024]` et `[1024,256]`, puis vérifie l’acceptation nominale et le rejet d’un axe QKV `[256,512]`. Les validations structurelles du lot 85 et les tests de bornes restent actifs. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le prochain lot peut relier ces contrats aux descripteurs quantifiés réels et vérifier que les tailles de données FAT16 correspondent au produit des axes et aux tailles Q-K avant d’appeler les kernels de produit scalaire.
