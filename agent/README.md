# Runtime agent MOHHDY (ASSIST-050 a 061)

Arborescence parallèle au prototype i386. Origine HTTP, widget d'embed,
sessions visiteur isolees, base autorisee locale, masque de droits,
escalade, handoff humain, simulateur de gestes DOM, facture mock,
vue navigateur d'instance (`/browser`) et FS sandbox (`/browser/fs`).
Install native, recette hyperviseur et mode hosted (scaffold, pas de
facturation). Les reponses sont un **stub local** (echo ou extraits de
KB), pas un LLM de production. Pas d'appel OpenAI. Pas Chromium. Pas
US-031. Pas le noyau Multiboot.

Guides :

- [../docs/assist050_docker_runtime.md](../docs/assist050_docker_runtime.md)
- [../docs/assist051_052_053_deploy.md](../docs/assist051_052_053_deploy.md)
- [../docs/assist060_061_browser.md](../docs/assist060_061_browser.md)
- [../docs/assist010_sessions_admin.md](../docs/assist010_sessions_admin.md)
- [../docs/assist012_droits_handoff.md](../docs/assist012_droits_handoff.md)
- [../docs/assist020_gestes_mcp.md](../docs/assist020_gestes_mcp.md)

Spec : [../US/mohhdy_agent_support_web.md](../US/mohhdy_agent_support_web.md).

```text
python3 agent/server.py
MOHHDY_AGENT_CONFIG=agent/config.example.json python3 agent/server.py
make agent-smoke
make agent-install-check
make agent-hypervisor-dry-run
make agent-docker
docker run --rm -p 8080:8080 mohhdy-agent
docker run --rm -p 8080:8080 -e ADMIN_TOKEN=change-me-at-runtime mohhdy-agent
docker run --rm -p 8080:8080 \
  -e ADMIN_TOKEN=change-me-at-runtime \
  -e MOHHDY_AGENT_CONFIG=/app/config.example.json \
  -e MOHHDY_AGENT_MODE=self_host \
  -e MOHHDY_AGENT_RUNTIME=docker \
  mohhdy-agent
agent/scripts/install.sh --prefix /tmp/mohhdy-agent --start --port 18080
```
