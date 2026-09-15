# AOS-361 à AOS-368 — extraction JSON de réponses LLM

`net_json_extract_string` récupère une valeur JSON string associée à une clé ASCII dans un buffer caller-owned. Il est prévu pour extraire un champ simple d’un body HTTP déjà déchiffré et validé, par exemple `response` pour une réponse locale de type Ollama ou un champ de texte adapté par la couche appelante.

Le décodeur accepte les échappements JSON `\"`, `\\`, `\/`, `\b`, `\f`, `\n`, `\r` et `\t`. Il rejette les caractères de contrôle bruts, les échappements `\uXXXX`, les strings non terminées, les clés absentes et les sorties dépassant le buffer fourni.

Le test couvre l’extraction de `response`, le décodage d’un saut de ligne, une clé absente, `\u0041` non pris en charge et une capacité de sortie insuffisante.

> Ce module n’est pas un parseur JSON complet. Il ne traite ni Unicode, ni nombres, ni booléens, ni tableaux, ni objets imbriqués de façon sémantique, ni streaming SSE. Les formats OpenAI/Ollama spécifiques doivent rester explicitement adaptés par l’appelant.
