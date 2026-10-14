# One image for both services of compose.yaml: the FTP server (the default command) and
# the exporter (python /app/export.py).
FROM python:3.13.7-alpine3.22

RUN pip install --no-cache-dir pyftpdlib==2.2.0

COPY ftpserver.py export.py render.py /app/

USER nobody
WORKDIR /app
ENV PYTHONUNBUFFERED=1 \
    FTP_ROOT=/export \
    FTP_HOST=0.0.0.0 \
    FTP_PORT=2121 \
    FTP_PASSIVE=30010-30019 \
    EXPORT_DIR=/export

EXPOSE 2121 30010-30019
CMD ["python", "/app/ftpserver.py"]
