# AOS-953 à AOS-960 — rotation multi-fournisseur après épuisement du budget

Le chemin LLM expose une rotation explicite entre les deux fournisseurs pris en charge. `ne2k_llm_connection_rotate_provider` ne change de fournisseur qu’après épuisement du budget caller-owned ; avant cette échéance, il retourne zéro et conserve la valeur. Les identifiants inconnus sont rejetés.

La rotation ne copie ni bearer, ni hôte, ni chemin, ni modèle. Le caller doit fournir les paramètres du fournisseur suivant et peut réutiliser le GET de reprise `Last-Event-ID` lorsque sa politique l’autorise. Cette séparation évite une fuite de secret ou une bascule implicite non contrôlée.

Le test vérifie le maintien du fournisseur avant épuisement, la bascule Ollama/OpenAI après épuisement et le rejet d’une valeur inconnue. Le prochain axe concerne la persistance inter-session minimale de l’état de reprise.

Auteur : **Manus AI**
