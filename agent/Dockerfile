# Image runtime HTTP agent. Aucun secret, aucun .env, aucun noyau i386.
# ADMIN_TOKEN se passe au run (-e), jamais ici.
FROM python:3.12-slim-bookworm

RUN useradd --create-home --uid 10001 --shell /usr/sbin/nologin mohhdy

WORKDIR /app

COPY --chown=mohhdy:mohhdy server.py ./
COPY --chown=mohhdy:mohhdy static ./static

USER mohhdy

ENV MOHHDY_AGENT_HOST=0.0.0.0
ENV MOHHDY_AGENT_PORT=8080
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD python3 -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8080/health')"

CMD ["python3", "-u", "server.py"]
