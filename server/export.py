"""Exports the public pages of the wiki for TTGWIKI, the DOS wiki reader.

Reads the Grav wiki's JSON API (/custom/sitemap.json and /custom/page.json, the same
endpoints the wiki BBS reads), renders every public page as text (render.py) and writes
the export the FTP server shares:

    WIKI.INI          format, generation, when, languages, line width
    EN/INDEX.TXT      the page tree, one page per line
    EN/FILES.LST      "crc32 size path" of INDEX.TXT and the pages (as DOSINST's)
    EN/P0000001.TXT   a page
    HU/...

The text files are UTF-8 with CRLF line ends. INDEX.TXT has tab-separated fields:

    id  parent-id  depth  modified  locale  route  title

in tree order (a parent before its children; 0 is no parent). A page file starts with
header lines (TITLE=, ROUTE=, LOCALE=, MODIFIED=, URL=, and L=<n> <target> for each
link: '#<id>' for a page of the export, a URL for anything else), then an empty line,
then the lines of the page (see render.py).

A page keeps its id for good: the ids are read back from the INDEX.TXT files of the
export, new routes get the next free number. Private pages (login-protected on the
wiki) are never exported: they are dropped from the sitemap, and without a token Grav
does not give out their text anyway.

An export is written only when something changed: the new tree goes into .next, the
language folders are swapped in with renames, and WIKI.INI comes last with the next
generation number, so a client that sees a new generation finds the files in place.
A failed run (the wiki down, a page that cannot be read) changes nothing.

Settings come from the environment:

    WIKI_URL      where the exporter reads the wiki        (default: http://grav)
    PUBLIC_URL    the wiki's address for people            (default: https://wiki.teletypegames.org)
    EXPORT_DIR    the export folder (the FTP root)         (default: export)
    LANGUAGES     the languages, the default one first     (default: en,hu)
    INTERVAL      seconds between two runs, 0: run once    (default: 900)
"""
import datetime
import json
import os
import shutil
import sys
import time
import traceback
import urllib.parse
import urllib.request
import zlib

from render import WIDTH, render

FORMAT = 1
TIMEOUT = 30


def log(message):
    print("[export] %s" % message, flush=True)


class FetchError(Exception):
    pass


class Wiki:
    def __init__(self, base_url):
        self.base_url = base_url.rstrip("/")

    def get(self, prefix, path, **query):
        url = "%s%s%s" % (self.base_url, prefix, path)
        if query:
            url += "?" + urllib.parse.urlencode(query)
        try:
            with urllib.request.urlopen(url, timeout=TIMEOUT) as response:
                return json.loads(response.read().decode("utf-8"))
        except Exception as error:
            raise FetchError("%s: %s" % (url, error)) from error

    def sitemap(self, prefix):
        pages = self.get(prefix, "/custom/sitemap.json").get("pages")
        if not isinstance(pages, list):
            raise FetchError("no pages in the sitemap of %s" % (prefix or "/"))
        return pages

    def page(self, prefix, route):
        return self.get(prefix, "/custom/page.json", route=route)


def prefix_of(lang, default):
    return "" if lang == default else "/" + lang


def read_ids(export_dir, languages):
    """route -> id from the INDEX.TXT files of the current export."""
    ids = {}
    for lang in languages:
        path = os.path.join(export_dir, lang.upper(), "INDEX.TXT")
        if not os.path.exists(path):
            continue
        with open(path, encoding="utf-8") as f:
            for line in f:
                fields = line.rstrip("\r\n").split("\t")
                if len(fields) >= 7 and fields[0].isdigit():
                    ids[fields[5]] = int(fields[0])
    return ids


def assign_ids(ids, routes):
    next_id = max(ids.values(), default=0) + 1
    for route in routes:
        if route not in ids:
            ids[route] = next_id
            next_id += 1
    return ids


def parent_route(route, routes):
    """The nearest exported ancestor of route; '/' is the root of everything."""
    if route == "/":
        return None
    path = route
    while True:
        path = path.rsplit("/", 1)[0] or "/"
        if path in routes:
            return path
        if path == "/":
            return None


def tree_order(pages):
    """The pages (in sitemap order) so a parent comes before its children: (page, parent route, depth)."""
    routes = {p["route"] for p in pages}
    children = {}
    roots = []
    for page in pages:
        parent = parent_route(page["route"], routes)
        if parent is None:
            roots.append(page)
        else:
            children.setdefault(parent, []).append(page)
    roots.sort(key=lambda p: p["route"] != "/")
    ordered = []

    def visit(page, parent, depth):
        ordered.append((page, parent, depth))
        for child in children.get(page["route"], []):
            visit(child, page["route"], depth + 1)

    for root in roots:
        visit(root, None, 0)
    return ordered


def single_line(text):
    return " ".join(str(text or "").replace("\t", " ").split())


def page_file(page, data, lines, targets, url):
    header = [
        "TITLE=%s" % single_line(data.get("title") or page.get("title")),
        "ROUTE=%s" % page["route"],
        "LOCALE=%s" % single_line(data.get("locale") or page.get("locale")),
        "MODIFIED=%d" % int(data.get("modified") or page.get("modified") or 0),
        "URL=%s" % url,
    ]
    header += ["L=%d %s" % (i + 1, target) for i, target in enumerate(targets)]
    return crlf(header + [""] + lines)


def crlf(lines):
    return "".join(line + "\r\n" for line in lines).encode("utf-8")


def files_list(files):
    lines = []
    for name in sorted(files):
        data = files[name]
        lines.append("%08x %d %s" % (zlib.crc32(data) & 0xFFFFFFFF, len(data), name))
    return crlf(lines)


def export_language(wiki, lang, prefix, public_url, ids, sitemap):
    """The files of one language folder: name -> bytes."""
    rendered = []
    for page in sitemap:
        data = wiki.page(prefix, page["route"])
        error = data.get("error")
        if error in ("forbidden", "not_found"):
            log("%s %s: %s, skipped" % (lang, page["route"], error))
            continue
        if error or data.get("private"):
            raise FetchError("%s %s: %s" % (lang, page["route"], error or "private"))
        rendered.append((page, data))
    routes = {page["route"]: ids[page["route"]] for page, _ in rendered}
    files = {}
    index = []
    for page, parent, depth in tree_order([p for p, _ in rendered]):
        data = next(d for p, d in rendered if p is page)
        page_id = ids[page["route"]]
        lines, targets = render(data.get("html") or "", routes, lang, public_url)
        url = public_url.rstrip("/") + prefix + (page["route"] if page["route"] != "/" else "/")
        files["P%07d.TXT" % page_id] = page_file(page, data, lines, targets, url)
        index.append("\t".join([
            str(page_id), str(ids[parent]) if parent else "0", str(depth),
            str(int(data.get("modified") or page.get("modified") or 0)),
            single_line(data.get("locale") or page.get("locale")), page["route"],
            single_line(data.get("title") or page.get("title")),
        ]))
    files["INDEX.TXT"] = crlf(index)
    files["FILES.LST"] = files_list(files)
    return files


def read_tree(folder):
    files = {}
    if os.path.isdir(folder):
        for name in os.listdir(folder):
            path = os.path.join(folder, name)
            if os.path.isfile(path):
                with open(path, "rb") as f:
                    files[name] = f.read()
    return files


def read_generation(export_dir):
    path = os.path.join(export_dir, "WIKI.INI")
    if not os.path.exists(path):
        return 0
    with open(path, encoding="utf-8") as f:
        for line in f:
            key, _, value = line.strip().partition("=")
            if key == "generation" and value.isdigit():
                return int(value)
    return 0


def write_file(path, data):
    temp = path + ".tmp"
    with open(temp, "wb") as f:
        f.write(data)
    os.chmod(temp, 0o644)
    os.replace(temp, path)


def publish(export_dir, trees, languages, generation):
    staging = os.path.join(export_dir, ".next")
    shutil.rmtree(staging, ignore_errors=True)
    os.makedirs(staging)
    for lang in languages:
        folder = os.path.join(staging, lang.upper())
        os.makedirs(folder)
        os.chmod(folder, 0o755)
        for name, data in trees[lang].items():
            write_file(os.path.join(folder, name), data)
    for lang in languages:
        current = os.path.join(export_dir, lang.upper())
        old = os.path.join(export_dir, ".old-" + lang.upper())
        shutil.rmtree(old, ignore_errors=True)
        if os.path.isdir(current):
            os.rename(current, old)
        os.rename(os.path.join(staging, lang.upper()), current)
        shutil.rmtree(old, ignore_errors=True)
    shutil.rmtree(staging, ignore_errors=True)
    exported = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    ini = crlf([
        "[wiki]",
        "format=%d" % FORMAT,
        "generation=%d" % generation,
        "exported=%s" % exported,
        "languages=%s" % ",".join(lang.upper() for lang in languages),
        "width=%d" % WIDTH,
    ])
    write_file(os.path.join(export_dir, "WIKI.INI"), ini)


def run_once(wiki, export_dir, languages, public_url):
    """True when a new export was published."""
    os.makedirs(export_dir, exist_ok=True)
    sitemaps = {}
    for lang in languages:
        sitemaps[lang] = [p for p in wiki.sitemap(prefix_of(lang, languages[0]))
                          if not p.get("private") and p.get("route")]
    if not any(sitemaps.values()):
        raise FetchError("the sitemap has no public pages")
    ids = read_ids(export_dir, languages)
    for lang in languages:
        assign_ids(ids, [p["route"] for p in sitemaps[lang]])
    trees = {}
    for lang in languages:
        trees[lang] = export_language(wiki, lang, prefix_of(lang, languages[0]), public_url, ids, sitemaps[lang])
    unchanged = all(read_tree(os.path.join(export_dir, lang.upper())) == trees[lang] for lang in languages)
    if unchanged and os.path.exists(os.path.join(export_dir, "WIKI.INI")):
        log("no change (generation %d)" % read_generation(export_dir))
        return False
    generation = read_generation(export_dir) + 1
    publish(export_dir, trees, languages, generation)
    counts = ", ".join("%s %d pages" % (lang, len(trees[lang]) - 2) for lang in languages)
    log("published generation %d: %s" % (generation, counts))
    return True


def main():
    wiki = Wiki(os.environ.get("WIKI_URL", "http://grav"))
    public_url = os.environ.get("PUBLIC_URL", "https://wiki.teletypegames.org")
    export_dir = os.environ.get("EXPORT_DIR", "export")
    languages = [lang.strip().lower() for lang in os.environ.get("LANGUAGES", "en,hu").split(",") if lang.strip()]
    interval = int(os.environ.get("INTERVAL", "900"))
    while True:
        try:
            run_once(wiki, export_dir, languages, public_url)
        except Exception as error:
            log("failed, the export stays as it was: %s" % error)
            if not isinstance(error, FetchError):
                traceback.print_exc()
            if interval == 0:
                return 1
        if interval == 0:
            return 0
        time.sleep(interval)


if __name__ == "__main__":
    sys.exit(main())
