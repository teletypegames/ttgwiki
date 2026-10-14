"""Writes a small export for the client's tests, with the real exporter on a fake wiki:

    python3 fixture.py EXPORT_DIR [VERSION]

VERSION (default 1) changes the DOS page, so a second run makes a new generation.
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.dirname(__file__))

import export  # noqa: E402
from test_export import FakeWiki  # noqa: E402


def wiki(version):
    w = FakeWiki()
    w.add("/", "Home", '<h1>Welcome</h1><p>Read about <a href="/dos">the DOS tools</a> or '
          '<a href="https://example.com/">the web</a>.</p>')
    w.add("/dos", "DOS tools", "<p>DOS page version %d.</p><ul><li><a href=\"/dos/gmenu\">GMENU</a></li></ul>"
          "<table><tr><th>Tool</th><th>Does</th></tr><tr><td>GMENU</td><td>menu</td></tr></table>" % version)
    w.add("/dos/gmenu", "GMENU", "<p>A game menu for DOS.</p><pre><code>GMENU.BAT</code></pre>")
    w.add("/secret", "Secret", "<p>never exported</p>")
    w.private.add("/secret")
    w.add("/", "Kezdőlap", "<p>Üdvözlünk, őszült fejű róka.</p>"
          '<p><a href="/dos">DOS</a></p>', "/hu")
    w.add("/dos", "DOS-eszközök", "<p>Magyar DOS oldal.</p>", "/hu")
    w.add("/dos/gmenu", "GMENU", "<p>Játékmenü.</p>", "/hu")
    return w


if __name__ == "__main__":
    export.run_once(wiki(int(sys.argv[2]) if len(sys.argv) > 2 else 1), sys.argv[1], ["en", "hu"],
                    "https://wiki.example")
