"""The FTP server of the end-to-end tests (needs pyftpdlib): anonymous, read-only, passive
only, like the live one (infra/ftp-server), whose root has the export in WIKI.
Settings come from the environment:

    FTP_ROOT          the folder to serve                 (default: .)
    FTP_HOST          address to listen on                 (default: 127.0.0.1)
    FTP_PORT          control port                         (default: 2121)
    FTP_PASSIVE       passive port range, "first-last"     (default: 30010-30019)
    FTP_MASQUERADE    public IP to announce in PASV replies (default: none)
    FTP_MAX_CONS      connections in total                 (default: 50)
    FTP_MAX_PER_IP    connections from one IP              (default: 5)
"""
import os

from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.handlers import FTPHandler
from pyftpdlib.servers import FTPServer


class PassiveOnlyHandler(FTPHandler):
    banner = "TTGWIKI test server ready."

    def ftp_PORT(self, line):
        self.respond("502 Active mode is disabled, use PASV.")

    def ftp_EPRT(self, line):
        self.respond("502 Active mode is disabled, use EPSV or PASV.")


def main():
    root = os.environ.get("FTP_ROOT", ".")
    first, last = (int(p) for p in os.environ.get("FTP_PASSIVE", "30010-30019").split("-"))

    authorizer = DummyAuthorizer()
    authorizer.add_anonymous(os.path.abspath(root), perm="elr")

    handler = PassiveOnlyHandler
    handler.authorizer = authorizer
    handler.passive_ports = range(first, last + 1)
    handler.masquerade_address = os.environ.get("FTP_MASQUERADE") or None
    handler.timeout = 120

    server = FTPServer((os.environ.get("FTP_HOST", "127.0.0.1"), int(os.environ.get("FTP_PORT", "2121"))), handler)
    server.max_cons = int(os.environ.get("FTP_MAX_CONS", "50"))
    server.max_cons_per_ip = int(os.environ.get("FTP_MAX_PER_IP", "5"))
    server.serve_forever()


if __name__ == "__main__":
    main()
