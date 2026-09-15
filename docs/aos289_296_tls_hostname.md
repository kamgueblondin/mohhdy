# AOS-289 à AOS-296 — validation de nom d’hôte X.509/TLS bornée

## Périmètre livré

Le parseur X.509 caller-owned expose désormais deux vues supplémentaires : le premier `commonName` (`CN`) du subject et le contenu de l’extension `subjectAltName` lorsqu’elle est présente. La fonction `x509_certificate_hostname_validate` compare ensuite un nom DNS ASCII nul-terminé avec les identités du certificat, sans copie persistante ni allocation dynamique.

| Règle | Comportement |
|---|---|
| `subjectAltName` dNSName | Les identités de tag DER `0x82` sont examinées. Une correspondance valide retourne le succès. |
| Priorité SAN | Dès qu’au moins un `dNSName` est présent, le `CN` n’est plus utilisé comme repli. |
| Repli CN | En l’absence de `dNSName`, le `commonName` est comparé s’il est encodé en UTF8String, PrintableString ou IA5String. |
| Casse | La comparaison des noms DNS ASCII est insensible à la casse. |
| Wildcard | Seul un wildcard complet de la forme `*.suffix` est admis ; il couvre exactement un label gauche. |
| Bornes | Noms DNS limités à 253 octets, labels à 63 octets et caractères ASCII DNS `[A-Za-z0-9-]`. |

## Rejets et comportement transactionnel

Le code rejette les noms vides, les labels vides ou trop longs, les tirets en début ou fin de label, les caractères hors DNS, les formes wildcard partielles, les noms à plusieurs labels sous un wildcard et les encodages DER d’extension incohérents. Lors du parsing, une seconde extension SAN ou un second CN est rejeté afin de conserver une identité non ambiguë dans les vues publiées.

Le vecteur de test contient un CN `wrong.example` et un SAN `api.example.test`. Il confirme la priorité du SAN, l’égalité insensible à la casse, le rejet de l’ancien CN tant qu’un dNSName existe, le fallback CN sans SAN et la règle wildcard à un seul label.

> La comparaison hostname authentifie seulement la liaison entre un nom demandé et l’identité déclarée par un certificat déjà parsé. Elle ne confère **aucune confiance** tant que la chaîne, les signatures de certificats, les dates et les usages de clé ne sont pas validés.

## Contrat mémoire

Le certificat DER, les vues X.509 et la chaîne hostname restent entièrement détenus par l’appelant. La fonction lit des vues et une chaîne nul-terminée bornée, et ne recourt ni à `kmalloc` ni à un buffer interne.

## Limites explicites

Les noms IP (`iPAddress`), IDNA/punycode, Unicode, `nameConstraints`, `keyUsage`/`extendedKeyUsage`, les noms de certificat client, la vérification de chaîne, les ancres de confiance, les dates et les signatures de certificats ne sont pas implémentés. Cette API n’est pas encore appelée automatiquement depuis le chemin de handshake du pilote NE2000 ; elle doit être invoquée par l’orchestrateur TLS après parsing du certificat serveur. Le backend X25519/bigint reste non constante-temps.
