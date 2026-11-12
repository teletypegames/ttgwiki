# The exporter of the wiki: it writes the export that the Teletype Games FTP server
# (infra/ftp-server) shares as WIKI.
FROM python:3.13.18-alpine3.24

COPY export.py render.py /app/

USER nobody
WORKDIR /app
ENV PYTHONUNBUFFERED=1 \
    EXPORT_DIR=/export

CMD ["python", "/app/export.py"]
