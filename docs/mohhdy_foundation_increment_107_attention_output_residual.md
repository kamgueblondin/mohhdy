# MOHHDY Foundation — Incrément 107 : projection de sortie d’attention

**État :** implémenté et testé.

## Objectif

Le lot 107 ajoute `gpt2_gguf_attention_output_add_residual_fat16`. La concaténation des sorties de têtes est projetée par la matrice GGUF `attn_output.weight`, reçoit un biais optionnel, puis est ajoutée au résiduel caller-owned.

## Contrat

La matrice de sortie doit être carrée `[channels, channels]`. Le scratch quantifié est fourni par l’appelant, tout comme le vecteur `projected` et le résiduel. Les capacités sont exprimées en floats pour les sorties et en octets pour le scratch de lecture de lignes, conformément aux primitives de projection existantes.

## Validation

Les tests vérifient le rejet d’une forme non carrée et d’une capacité de résiduel insuffisante. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le sous-bloc attention dispose maintenant de la dernière étape : concaténation multi-têtes, projection de sortie, biais optionnel et résiduel. Il peut être composé avec LayerNorm pré-attention puis avec le sous-bloc MLP pré-normé.
