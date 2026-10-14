import os
import shutil
import sys
import tempfile
import unittest
import zlib

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

import export  # noqa: E402


class FakeWiki:
    """The wiki's JSON API: sitemaps and pages by language prefix."""

    def __init__(self):
        self.pages = {"": {}, "/hu": {}}
        self.private = set()
        self.failing = False

    def add(self, route, title, html, prefix="", modified=100):
        self.pages[prefix][route] = {"route": route, "title": title, "locale": prefix.strip("/") or "en",
                                     "modified": modified, "html": html}

    def sitemap(self, prefix):
        if self.failing:
            raise export.FetchError("wiki down")
        return [dict(p, private=p["route"] in self.private) for p in self.pages[prefix].values()]

    def page(self, prefix, route):
        if route in self.private:
            return {"error": "forbidden"}
        return dict(self.pages[prefix][route], private=False)


class ExportTest(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.mkdtemp()
        self.wiki = FakeWiki()
        for prefix in ("", "/hu"):
            self.wiki.add("/", "Home", '<p>See <a href="/dos">DOS</a> and <a href="/secret">it</a></p>', prefix)
            self.wiki.add("/dos", "DOS", "<p>dos</p>", prefix)
            self.wiki.add("/dos/gmenu", "GMENU", "<h1>GMENU</h1>", prefix)
            self.wiki.add("/secret", "Secret", "<p>no</p>", prefix)
            self.wiki.add("/secret/open", "Open", "<p>under a private page</p>", prefix)
        self.wiki.private.add("/secret")

    def tearDown(self):
        shutil.rmtree(self.dir)

    def run_export(self):
        return export.run_once(self.wiki, self.dir, ["en", "hu"], "https://wiki.example")

    def read(self, *path):
        with open(os.path.join(self.dir, *path), "rb") as f:
            return f.read().decode("utf-8")

    def index(self, lang="EN"):
        return [line.split("\t") for line in self.read(lang, "INDEX.TXT").split("\r\n") if line]

    def test_first_export(self):
        self.assertTrue(self.run_export())
        self.assertIn("generation=1\r\n", self.read("WIKI.INI"))
        rows = self.index()
        self.assertEqual([r[5] for r in rows], ["/", "/dos", "/dos/gmenu", "/secret/open"])
        ids = {r[5]: r[0] for r in rows}
        self.assertEqual(rows[0][1:3], ["0", "0"])
        self.assertEqual(rows[2][1:3], [ids["/dos"], "2"])
        self.assertEqual(rows[3][1:3], [ids["/"], "1"], "a page under a private one hangs from the nearest public")

    def test_private_pages_are_never_written(self):
        self.run_export()
        for lang in ("EN", "HU"):
            names = os.listdir(os.path.join(self.dir, lang))
            self.assertEqual(len(names), 6)
            for name in names:
                self.assertNotIn("no</p>", self.read(lang, name))
                self.assertNotIn("Secret", self.read(lang, name))

    def test_page_file(self):
        self.run_export()
        ids = {r[5]: r[0] for r in self.index("HU")}
        page = self.read("HU", "P%07d.TXT" % int(ids["/"]))
        header, body = page.split("\r\n\r\n", 1)
        self.assertIn("TITLE=Home", header)
        self.assertIn("URL=https://wiki.example/hu/", header)
        self.assertIn("L=1 #%s" % ids["/dos"], header)
        self.assertNotIn("L=2", header)
        self.assertTrue(body.endswith("\r\n"))

    def test_files_list_matches_the_files(self):
        self.run_export()
        for line in self.read("EN", "FILES.LST").split("\r\n"):
            if not line:
                continue
            crc, size, name = line.split(" ")
            with open(os.path.join(self.dir, "EN", name), "rb") as f:
                data = f.read()
            self.assertEqual(int(crc, 16), zlib.crc32(data))
            self.assertEqual(int(size), len(data))

    def test_unchanged_wiki_keeps_the_generation(self):
        self.run_export()
        self.assertFalse(self.run_export())
        self.assertIn("generation=1\r\n", self.read("WIKI.INI"))

    def test_change_makes_a_new_generation_and_ids_stay(self):
        self.run_export()
        before = {r[5]: r[0] for r in self.index()}
        del self.wiki.pages[""]["/dos"]
        self.wiki.add("/new", "New", "<p>new</p>")
        self.wiki.add("/dos/gmenu", "GMENU", "<h1>GMENU 2</h1>", modified=200)
        self.assertTrue(self.run_export())
        self.assertIn("generation=2\r\n", self.read("WIKI.INI"))
        after = {r[5]: r[0] for r in self.index()}
        self.assertEqual(after["/dos/gmenu"], before["/dos/gmenu"])
        self.assertEqual(int(after["/new"]), max(int(v) for v in before.values()) + 1)
        self.assertNotIn("/dos", after)
        self.assertFalse(os.path.exists(os.path.join(self.dir, "EN", "P%07d.TXT" % int(before["/dos"]))))
        self.assertEqual(sorted(os.listdir(self.dir)), ["EN", "HU", "WIKI.INI"])

    def test_a_failed_run_changes_nothing(self):
        self.run_export()
        self.wiki.failing = True
        with self.assertRaises(export.FetchError):
            self.run_export()
        self.assertIn("generation=1\r\n", self.read("WIKI.INI"))
        self.assertEqual(len(self.index()), 4)


if __name__ == "__main__":
    unittest.main()
