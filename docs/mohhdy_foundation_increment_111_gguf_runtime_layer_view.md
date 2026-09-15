# MOHHDY Foundation — Incrément 111 : vue runtime d’une couche GGUF

**État :** implémenté et testé.

## Objectif

Le lot 111 ajoute `gpt2_gguf_runtime_get_layer`, qui fournit au forward une copie contrôlée du descripteur d’une couche déjà préparée par `gpt2_gguf_runtime_prepare`. Le blob GGUF n’est pas rescanné et aucun pointeur interne n’est transféré à l’appelant.

## Contrat

La primitive exige un runtime marqué `ready`, une table de couches et une sortie non nuls. Un index supérieur ou égal à `layer_count` est rejeté. La sortie est une vue structurelle par valeur; les buffers de données restent ceux du modèle chargé et ne sont pas copiés.

## Validation

Les tests couvrent un runtime nul et un runtime non préparé. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.
