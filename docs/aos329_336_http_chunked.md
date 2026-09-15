# AOS-329 à AOS-336 — réponses HTTP chunked sur TLS

Le module HTTP/TLS fournit désormais `net_http_chunked_accumulator_t`. Il accepte une réponse HTTP/1.1 avec `Transfer-Encoding: chunked` exactement, décode les tailles hexadécimales bornées, compacte les octets de body dans le buffer caller-owned et publie la réponse uniquement après la terminaison `0\r\n\r\n`.

La machine à états conserve séparément la ligne de taille, les octets de chunk restants, le CRLF obligatoire et la terminaison. Elle accepte les tailles et les corps répartis sur plusieurs plaintexts, sans allocation dynamique ni copie dans un état caché. Une taille non hexadécimale, un dépassement de buffer, un CRLF invalide, une taille au-delà de 65 535 ou un header non chunked est rejeté.

Le test couvre une réponse `200` avec body `Wikipedia` réparti en deux fragments, les chunks `4` et `5`, ainsi qu’une taille `Z` invalide.

> Les extensions de chunk, trailers non vides, codages de transfert combinés, compression, HTTP/2, streaming SSE, gros bodies et le wrapper TLS/NE2000 spécialisé chunked ne sont pas encore fournis.
