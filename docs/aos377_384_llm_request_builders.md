# AOS-377 à AOS-384 — builders JSON de requêtes LLM

`net_llm_build_ollama_generate_json` construit un body non-streaming Ollama de la forme `{"model":"…","prompt":"…","stream":false}`. `net_llm_build_openai_chat_json` construit un body OpenAI compatible de la forme `{"model":"…","messages":[{"role":"user","content":"…"}],"stream":false}`.

Les modèles et prompts restent des entrées caller-owned. Les guillemets, antislashs, retours à la ligne, retours chariot et tabulations sont échappés. Les contrôles non pris en charge et les octets non ASCII sont rejetés, de même que les capacités insuffisantes ; aucun buffer caché ni état de fournisseur n’est conservé.

Le test contrôle les octets exacts des deux bodies pour un prompt contenant guillemets et saut de ligne, puis vérifie les rejets d’un octet de contrôle et d’une capacité limitée.

> Les builders ne mettent pas encore ces bodies dans un POST HTTP signé d’un header Bearer propre à un fournisseur. Unicode `\uXXXX`, paramètres de génération, image/audio, outils, messages système, multi-tours et streaming SSE restent hors périmètre.
