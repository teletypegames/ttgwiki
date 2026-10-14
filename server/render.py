"""A wiki page's HTML (Grav's rendered page.content) as the text TTGWIKI shows.

The text is cut into lines of at most WIDTH characters (Unicode code points: the client
turns each into one byte of its code page). A line is complete in itself: it starts in
the normal style, and its control bytes say where the style changes and where links are:

    01h c        the style from here on: c is one of STYLES
    02h n 03h    a link starts, n is its number in decimal (the page's L= lines)
    04h          the link ends

A style that runs over the end of a line starts again on the next one, and so does a
link. The client fills the rest of a line with the style it ended in.
"""
import re
import unicodedata
from html.parser import HTMLParser
from urllib.parse import urlsplit

WIDTH = 78

STYLE = "\x01"
LINK = "\x02"
LINK_NUMBER_END = "\x03"
LINK_END = "\x04"

NORMAL = "0"
STYLES = {
    "0": "normal text",
    "1": "heading 1",
    "2": "heading 2",
    "3": "heading 3 and lower",
    "b": "bold",
    "i": "italic",
    "c": "inline code",
    "p": "code block",
    "q": "quote",
    "r": "horizontal rule",
    "t": "table frame",
    "h": "table header",
}

IMAGE_LABEL = {"en": "Image", "hu": "Kép"}

BLOCK_TAGS = {"p", "div", "section", "article", "main", "header", "footer", "nav", "aside", "ul", "ol", "li",
              "pre", "blockquote", "table", "thead", "tbody", "tfoot", "tr", "td", "th", "h1", "h2", "h3",
              "h4", "h5", "h6", "hr", "dl", "dt", "dd", "figure", "figcaption", "details", "summary"}
VOID_TAGS = {"br", "hr", "img", "input", "meta", "link", "wbr", "col", "source"}
SKIP_TAGS = {"script", "style", "template", "svg", "button", "form", "iframe"}


class Node:
    def __init__(self, tag, attrs=None, parent=None):
        self.tag = tag
        self.attrs = dict(attrs or [])
        self.children = []
        self.parent = parent

    def text(self):
        return "".join(c if isinstance(c, str) else c.text() for c in self.children)


class TreeBuilder(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.root = Node("root")
        self.current = self.root
        self.skipping = 0

    def handle_starttag(self, tag, attrs):
        if self.skipping or tag in SKIP_TAGS:
            if tag not in VOID_TAGS:
                self.skipping += 1
            return
        node = Node(tag, attrs, self.current)
        self.current.children.append(node)
        if tag not in VOID_TAGS:
            self.current = node

    def handle_startendtag(self, tag, attrs):
        if self.skipping or tag in SKIP_TAGS:
            return
        self.current.children.append(Node(tag, attrs, self.current))

    def handle_endtag(self, tag):
        if self.skipping:
            self.skipping -= 1
            return
        node = self.current
        while node is not self.root and node.tag != tag:
            node = node.parent
        if node is not self.root:
            self.current = node.parent

    def handle_data(self, data):
        if not self.skipping:
            self.current.children.append(data)


def parse(html):
    builder = TreeBuilder()
    builder.feed(html)
    builder.close()
    return builder.root


def clean(text):
    text = unicodedata.normalize("NFC", text)
    return "".join(ch for ch in text if ch >= " " or ch in "\t\n")


class Cell:
    """One character of the output with its style and link (0: none)."""
    __slots__ = ("ch", "style", "link")

    def __init__(self, ch, style, link=0):
        self.ch = ch
        self.style = style
        self.link = link


def cells_of(text, style, link=0):
    return [Cell(ch, style, link) for ch in text]


class Links:
    """The page's links, numbered from 1; the same target keeps its number."""

    def __init__(self, routes, lang, base_url):
        self.routes = routes
        self.lang = lang
        self.host = urlsplit(base_url).netloc
        self.base_url = base_url.rstrip("/")
        self.targets = []

    def number(self, href):
        target = self.target(href)
        if target is None:
            return 0
        if target not in self.targets:
            self.targets.append(target)
        return self.targets.index(target) + 1

    def target(self, href):
        """'#id' for a page of the export, the URL for anything else, None for no link."""
        href = (href or "").strip()
        if not href or href.startswith("#") or href.lower().startswith("javascript:"):
            return None
        parts = urlsplit(href)
        if parts.scheme and parts.scheme not in ("http", "https"):
            return href
        if parts.netloc and parts.netloc != self.host:
            return href
        path = parts.path
        if not path.startswith("/"):
            return None
        if path.startswith("/wiki-assets/") or path.startswith("/user/") or path.startswith("/images/"):
            return self.base_url + path
        for prefix in ("/hu", "/en"):
            if path == prefix or path.startswith(prefix + "/"):
                path = path[len(prefix):] or "/"
                break
        route = path.rstrip("/") or "/"
        page_id = self.routes.get(route)
        if page_id is None:
            return None
        return "#%d" % page_id


def wrap(cells, width, first_prefix=None, next_prefix=None):
    """Word-wraps cells into lines of at most width cells, the prefixes included.
    A '\\n' cell is a forced line break."""
    first_prefix = first_prefix or []
    next_prefix = next_prefix if next_prefix is not None else [Cell(" ", NORMAL)] * len(first_prefix)
    lines = []
    for chunk in split_breaks(cells):
        line = list(first_prefix if not lines else next_prefix)
        room = max(1, width - len(line))
        used = 0
        for word, space in split_words(chunk):
            if used and used + len(space) + len(word) > room:
                lines.append(line)
                line = list(next_prefix)
                room = max(1, width - len(line))
                used = 0
            if not used:
                space = []
            while not used and len(word) > room:
                cut = break_point(word, room)
                lines.append(line + word[:cut])
                word = word[cut:]
                line = list(next_prefix)
                room = max(1, width - len(line))
            line.extend(space)
            line.extend(word)
            used += len(space) + len(word)
        lines.append(line)
    return lines


def break_point(word, room):
    """Where a word longer than room is cut: after a '-' or '/' if there is one."""
    for i in range(room - 1, 0, -1):
        if word[i].ch in "-/_.,":
            return i + 1
    return room


def split_breaks(cells):
    chunk = []
    for cell in cells:
        if cell.ch == "\n":
            yield chunk
            chunk = []
        else:
            chunk.append(cell)
    yield chunk


def split_words(cells):
    """(word, the spaces before it) pairs; spaces at the ends are dropped."""
    words = []
    space, word = [], []
    for cell in cells:
        if cell.ch == " ":
            if word:
                words.append((word, space))
                space, word = [], []
            space.append(cell)
        else:
            word.append(cell)
    if word:
        words.append((word, space))
    if words:
        words[0] = (words[0][0], [])
    return words


def collapse(cells):
    """Runs of white space as one space, as a browser shows them."""
    out = []
    for cell in cells:
        if cell.ch in " \t\r\n":
            if out and out[-1].ch == " ":
                continue
            cell = Cell(" ", cell.style, cell.link)
        out.append(cell)
    while out and out[0].ch == " ":
        out.pop(0)
    while out and out[-1].ch == " ":
        out.pop()
    for i, cell in enumerate(out):
        if cell.ch == " " and i + 1 < len(out) and out[i + 1].ch == "\n":
            cell.ch = ""
        if cell.ch == " " and i and out[i - 1].ch == "\n":
            cell.ch = ""
    return [c for c in out if c.ch]


def encode(line):
    """A line of cells as text with the control bytes."""
    out = []
    style, link = NORMAL, 0
    for cell in line:
        if cell.link != link:
            if link:
                out.append(LINK_END)
            if cell.link:
                out.append("%s%d%s" % (LINK, cell.link, LINK_NUMBER_END))
            link = cell.link
        if cell.style != style:
            out.append(STYLE + cell.style)
            style = cell.style
        out.append(cell.ch)
    if link:
        out.append(LINK_END)
    return "".join(out).rstrip(" ") if style == NORMAL else "".join(out)


class Renderer:
    def __init__(self, links, lang):
        self.links = links
        self.lang = lang
        self.lines = []

    def blank(self):
        if self.lines and self.lines[-1]:
            self.lines.append([])

    def emit(self, lines):
        self.lines.extend(lines)

    def inline(self, node, style, link=0):
        """The inline content of node as cells."""
        cells = []
        for child in node.children:
            if isinstance(child, str):
                cells.extend(cells_of(clean(child), style, link))
                continue
            tag = child.tag
            if tag in SKIP_TAGS:
                continue
            if tag == "br":
                cells.append(Cell("\n", style, link))
            elif tag == "img":
                alt = clean(child.attrs.get("alt") or "").strip()
                label = IMAGE_LABEL.get(self.lang, "Image")
                cells.extend(cells_of("[%s: %s]" % (label, alt) if alt else "[%s]" % label, "i", link))
            elif tag == "a":
                number = self.links.number(child.attrs.get("href")) or link
                cells.extend(self.inline(child, style, number))
            elif tag in ("strong", "b"):
                cells.extend(self.inline(child, "b" if style in (NORMAL, "q") else style, link))
            elif tag in ("em", "i", "cite", "dfn"):
                cells.extend(self.inline(child, "i" if style in (NORMAL, "q") else style, link))
            elif tag in ("code", "kbd", "samp", "tt"):
                cells.extend(self.inline(child, "c" if style in (NORMAL, "q", "b", "i") else style, link))
            elif tag in BLOCK_TAGS and tag not in ("td", "th"):
                cells.append(Cell(" ", style, link))
                cells.extend(self.inline(child, style, link))
                cells.append(Cell(" ", style, link))
            else:
                cells.extend(self.inline(child, style, link))
        return cells

    def paragraph(self, cells, width, first_prefix=None, next_prefix=None):
        cells = collapse(cells)
        if cells:
            self.emit(wrap(cells, width, first_prefix, next_prefix))

    def blocks(self, node, width, indent=None, style=NORMAL):
        """The children of node as blocks; inline runs between them become paragraphs."""
        indent = indent or []
        pending = []

        def flush():
            if pending:
                cells = []
                for item in pending:
                    if isinstance(item, str):
                        cells.extend(cells_of(clean(item), style))
                    else:
                        cells.extend(self.inline(wrapper(item), style))
                pending.clear()
                if collapse(list(cells)):
                    self.paragraph(cells, width, indent, indent)
                    self.blank()

        for child in node.children:
            if isinstance(child, str) or child.tag not in BLOCK_TAGS:
                pending.append(child)
                continue
            flush()
            self.block(child, width, indent, style)
        flush()

    def block(self, node, width, indent, style):
        tag = node.tag
        if tag in ("h1", "h2", "h3", "h4", "h5", "h6"):
            self.heading(node, width, indent)
        elif tag == "p":
            self.paragraph(self.inline(node, style), width, indent, indent)
            self.blank()
        elif tag in ("ul", "ol"):
            self.list(node, width, indent, style)
            self.blank()
        elif tag == "pre":
            self.code_block(node, width, indent)
            self.blank()
        elif tag == "blockquote":
            bar = indent + [Cell("│", "q"), Cell(" ", "q")]
            start = len(self.lines)
            self.blocks(node, width, bar, "q")
            while len(self.lines) > start and not self.lines[-1]:
                self.lines.pop()
            for i in range(start, len(self.lines)):
                if not self.lines[i]:
                    self.lines[i] = list(bar[:-1])
            self.blank()
        elif tag == "hr":
            self.blank()
            self.emit([list(indent) + cells_of("─" * (width - len(indent)), "r")])
            self.blank()
        elif tag == "table":
            self.table(node, width, indent)
            self.blank()
        elif tag in ("dt", "summary", "figcaption"):
            self.paragraph(self.inline(node, "b"), width, indent, indent)
        elif tag == "dd":
            self.blocks(node, width, indent + [Cell(" ", NORMAL)] * 4, style)
        else:
            self.blocks(node, width, indent, style)

    def heading(self, node, width, indent):
        level = int(node.tag[1])
        style = "1" if level == 1 else "2" if level == 2 else "3"
        cells = collapse(self.inline(node, style))
        for cell in cells:
            if cell.style in ("b", "i", "c"):
                cell.style = style
        if not cells:
            return
        self.blank()
        lines = wrap(cells, width, indent, indent)
        self.emit(lines)
        if level <= 2:
            longest = max(len(line) for line in lines) - len(indent)
            self.emit([list(indent) + cells_of(("═" if level == 1 else "─") * longest, style)])
        self.blank()

    def list(self, node, width, indent, style):
        number = int(node.attrs.get("start", "1") or 1) if node.tag == "ol" else 0
        items = [c for c in node.children if not isinstance(c, str) and c.tag == "li"]
        for item in items:
            if number:
                bullet = "%d. " % number
                number += 1
            else:
                bullet = "• "
            first = indent + cells_of(bullet, style)
            rest = indent + [Cell(" ", style)] * len(bullet)
            self.list_item(item, width, first, rest, style)

    def list_item(self, item, width, first, rest, style):
        pending = []
        prefix = [first]

        def flush():
            cells = []
            for child in pending:
                if isinstance(child, str):
                    cells.extend(cells_of(clean(child), style))
                elif child.tag == "p":
                    cells.extend(self.inline(child, style))
                    cells.append(Cell(" ", style))
                else:
                    cells.extend(self.inline(wrapper(child), style))
            pending.clear()
            if collapse(list(cells)):
                self.paragraph(cells, width, prefix[0], rest)
                prefix[0] = rest

        for child in item.children:
            if isinstance(child, str) or child.tag not in BLOCK_TAGS or child.tag == "p":
                pending.append(child)
                continue
            flush()
            if child.tag in ("ul", "ol"):
                if prefix[0] is first:
                    self.emit([list(first)])
                    prefix[0] = rest
                self.list(child, width, rest, style)
            else:
                start = len(self.lines)
                self.block(child, width, rest, style)
                if prefix[0] is first and len(self.lines) > start:
                    line = self.lines[start]
                    self.lines[start] = list(first) + line[len(rest):]
                    prefix[0] = rest
                while self.lines and not self.lines[-1]:
                    self.lines.pop()
        flush()
        if prefix[0] is first:
            self.emit([list(first)])

    def code_block(self, node, width, indent):
        text = clean(node.text()).expandtabs(4).rstrip("\n")
        if text.startswith("\n"):
            text = text[1:]
        pad = indent + [Cell(" ", "p")]
        room = width - len(pad) - 1
        for raw in text.split("\n"):
            raw = raw.rstrip()
            while True:
                if len(raw) <= room:
                    self.emit([pad + cells_of(raw.ljust(room) + " ", "p")])
                    break
                self.emit([pad + cells_of(raw[:room] + "»", "p")])
                raw = raw[room:]

    def table(self, node, width, indent):
        rows = []
        for tr in find_all(node, "tr"):
            cells = [c for c in tr.children if not isinstance(c, str) and c.tag in ("td", "th")]
            if not cells:
                continue
            header = all(c.tag == "th" for c in cells) or (tr.parent is not None and tr.parent.tag == "thead")
            style = "h" if header else NORMAL
            row = []
            for cell in cells:
                content = collapse(self.inline(cell, style))
                for c in content:
                    if c.ch == "\n":
                        c.ch = " "
                row.append(content)
            rows.append((header, row))
        if not rows:
            return
        columns = max(len(row) for _, row in rows)
        for _, row in rows:
            row.extend([[] for _ in range(columns - len(row))])
        widths = fit_columns(rows, columns, width - len(indent))
        if widths is None:
            self.table_as_records(rows, width, indent)
        else:
            self.table_as_grid(rows, widths, indent)

    def table_as_grid(self, rows, widths, indent):
        frame = "t"

        def rule(left, middle, right):
            cells = list(indent) + cells_of(left, frame)
            for i, w in enumerate(widths):
                cells.extend(cells_of("─" * (w + 2), frame))
                cells.extend(cells_of(middle if i + 1 < len(widths) else right, frame))
            return cells

        wrapped_rows = []
        for header, row in rows:
            wrapped = [wrap(cell, w) if cell else [[]] for cell, w in zip(row, widths)]
            wrapped_rows.append((header, wrapped))
        separate = any(max(len(c) for c in wrapped) > 1 for _, wrapped in wrapped_rows)

        self.emit([rule("┌", "┬", "┐")])
        for index, (header, wrapped) in enumerate(wrapped_rows):
            height = max(len(c) for c in wrapped)
            for y in range(height):
                cells = list(indent) + cells_of("│", frame)
                for cell_lines, w in zip(wrapped, widths):
                    content = cell_lines[y] if y < len(cell_lines) else []
                    fill = "h" if header else NORMAL
                    cells.append(Cell(" ", fill))
                    cells.extend(content)
                    cells.extend(cells_of(" " * (w - len(content) + 1), fill))
                    cells.extend(cells_of("│", frame))
                self.emit([cells])
            last = index + 1 == len(wrapped_rows)
            if not last and (header or separate):
                self.emit([rule("├", "┼", "┤")])
        self.emit([rule("└", "┴", "┘")])

    def table_as_records(self, rows, width, indent):
        """A table too wide for the screen: every row as "column: value" lines."""
        names = None
        for header, row in rows:
            if header and names is None:
                names = row
                continue
            self.emit([list(indent) + cells_of("─" * (width - len(indent)), "t")])
            for i, cell in enumerate(row):
                if not cell:
                    continue
                name = names[i] if names and i < len(names) else []
                label = [Cell(c.ch, "h", c.link) for c in name] + cells_of(": ", "h") if name else []
                self.paragraph(label + cell, width, indent, indent + [Cell(" ", NORMAL)] * 2)
        self.emit([list(indent) + cells_of("─" * (width - len(indent)), "t")])


def fit_columns(rows, columns, width):
    """The widths of the columns so the grid fits, or None when it cannot."""
    room = width - (3 * columns + 1)
    if room < columns * 3:
        return None
    longest = [0] * columns
    longest_word = [1] * columns
    for _, row in rows:
        for i, cell in enumerate(row):
            longest[i] = max(longest[i], len(cell))
            for word, _ in split_words(cell):
                longest_word[i] = max(longest_word[i], len(word))
    widths = [max(1, w) for w in longest]
    floor = [min(widths[i], max(6, min(longest_word[i], 14))) for i in range(columns)]
    while sum(widths) > room:
        candidates = [i for i in range(columns) if widths[i] > floor[i]]
        if not candidates:
            return None
        widest = max(candidates, key=lambda i: widths[i])
        widths[widest] -= 1
    return widths


def find_all(node, tag):
    for child in node.children:
        if isinstance(child, str):
            continue
        if child.tag == tag:
            yield child
        elif child.tag != "table":
            yield from find_all(child, tag)


def wrapper(node):
    holder = Node("span")
    holder.children = [node]
    return holder


def render(html, routes, lang, base_url, width=WIDTH):
    """(lines, link targets) of a page: lines encoded as described above."""
    links = Links(routes, lang, base_url)
    renderer = Renderer(links, lang)
    renderer.blocks(parse(html or ""), width)
    lines = renderer.lines
    while lines and not lines[-1]:
        lines.pop()
    while lines and not lines[0]:
        lines.pop(0)
    return [encode(line) for line in lines], links.targets


def visible(line):
    """A line without its control bytes (for the tests)."""
    return re.sub("\x01.|\x02[0-9]*\x03|\x04", "", line)
