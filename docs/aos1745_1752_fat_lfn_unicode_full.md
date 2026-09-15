# AOS-1745…1752 — LFN FAT16/FAT32 Unicode hors BMP

## Objet

Ce macro-lot complète le support UTF-8 des noms longs FAT en acceptant les caractères Unicode hors BMP. Les caractères encodés UTF-8 sur quatre octets sont convertis en une **paire de surrogates UTF-16LE** dans les entrées LFN FAT16 et FAT32, puis restitués dans leur forme UTF-8 d’origine lors de la lecture et du listage.

| Cas | Traitement |
|---|---|
| UTF-8 sur un à trois octets | Comportement compatible avec AOS-1737…1744 |
| UTF-8 sur quatre octets `U+10000…U+10FFFF` | Sérialisation en paire UTF-16LE haute/basse |
| Surrogate haut isolé | Rejet au décodage |
| Surrogate bas isolé | Rejet au décodage |
| UTF-8 surlong, tronqué ou au-delà de `U+10FFFF` | Rejet à l’encodage |

## Conception

`lfn_utf8_to_utf16_bmp()` conserve son contrat statique et borné, mais accepte désormais les séquences UTF-8 de quatre octets. Le point de code est décalé de `0x10000`, puis séparé en un surrogate haut `0xD800…0xDBFF` et un surrogate bas `0xDC00…0xDFFF`. La capacité est toujours exprimée en **unités UTF-16**, ce qui garantit qu’une paire n’est jamais écrite partiellement.

La conversion inverse contrôle qu’un surrogate haut est suivi d’un surrogate bas valide avant de publier les quatre octets UTF-8. Elle refuse également un surrogate bas isolé. Les entrées LFN existantes conservent leurs fragments de treize unités UTF-16LE : aucun changement n’est requis dans l’ordre FAT, le checksum 8.3 ou les interfaces FAT16/FAT32.

> Aucun appel à `kmalloc`, `malloc`, `calloc`, `realloc` ou `free` n’est introduit. Tous les tableaux restent bornés et appartiennent à l’appelant ou à la pile.

## Validation

Les tests introduisent le nom `rocket-😀.txt`, dont `U+1F600` est représenté par `D83D DE00` en UTF-16LE. Les scénarios FAT16 et FAT32 vérifient la sérialisation des quatre octets de la paire, la lecture de contenu par le nom UTF-8 et le nom restitué par le listage. Le test de conversion vérifie en outre le round-trip `A😀` et le refus d’un surrogate haut isolé.

| Vérification | Résultat |
|---|---|
| UTF-8 `F0 9F 98 80` vers `D83D DE00` | Réussie |
| Décodage de la paire vers UTF-8 | Réussi |
| Création, lecture et listage FAT16 `rocket-😀.txt` | Réussis |
| Création, lecture et listage FAT32 `rocket-😀.txt` | Réussis |
| Absence d’allocation dynamique dans le socle | Confirmée |
| `make -s test-all` | **462/462 tests réussis** |

## Limites

Le traitement reste volontairement limité aux noms LFN de taille bornée par `OS_NAME_MAX` et aux règles de chemin existantes. La comparaison de casse reste ASCII uniquement ; aucune normalisation Unicode, aucune transformation de casse linguistique et aucune allocation dynamique ne sont ajoutées.

## Références

[1] [Unicode Consortium — UTF-8 et UTF-16](https://www.unicode.org/standard/standard.html)  
[2] [Microsoft — Long File Names on FAT volumes](https://download.microsoft.com/download/1/2/7/127443f9-8bb5-40a6-8d72-a3c6e5bd12d5/FAT32_WhitePaper.pdf)
