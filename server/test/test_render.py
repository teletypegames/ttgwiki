import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from render import WIDTH, render, visible  # noqa: E402

BASE = "https://wiki.teletypegames.org"
ROUTES = {"/": 1, "/dos": 2, "/dos/gmenu": 3}


def text(html, lang="en", routes=ROUTES):
    lines, targets = render(html, routes, lang, BASE)
    return [visible(line) for line in lines], lines, targets


class RenderTest(unittest.TestCase):
    def test_paragraphs_wrap_at_the_width(self):
        shown, _, _ = text("<p>%s</p><p>Second.</p>" % ("word " * 40))
        self.assertTrue(all(len(line) <= WIDTH for line in shown))
        self.assertEqual(shown[-2:], ["", "Second."])
        self.assertTrue(shown[0].startswith("word word"))

    def test_headings_are_underlined(self):
        shown, lines, _ = text("<h1>Title</h1><p>x</p><h2>Sub</h2><h3>Small</h3>")
        self.assertEqual(shown[:3], ["Title", "═" * 5, ""])
        self.assertIn("─" * 3, shown)
        self.assertIn("Small", shown)
        self.assertTrue(lines[0].startswith("\x011Title"))

    def test_inline_styles_and_every_line_complete(self):
        _, lines, _ = text("<p><strong>%s</strong></p>" % ("bold " * 30))
        self.assertTrue(all(line.startswith("\x01b") for line in lines))

    def test_links_are_numbered_and_repeat_on_wrapped_lines(self):
        long = "linked text " * 10
        _, lines, targets = text('<p><a href="/dos/gmenu">%s</a> and <a href="https://example.com/x">out</a>'
                                 ' and <a href="/hu/dos/gmenu#x">again</a></p>' % long)
        self.assertEqual(targets, ["#3", "https://example.com/x"])
        self.assertTrue(lines[0].startswith("\x021\x03"))
        self.assertTrue(lines[1].startswith("\x021\x03"))
        self.assertTrue(all(line.count("\x02") == line.count("\x04") for line in lines))

    def test_private_and_anchor_links_are_plain_text(self):
        shown, lines, targets = text('<p><a href="/secret">hidden</a> <a href="#top">top</a></p>')
        self.assertEqual(targets, [])
        self.assertEqual(shown, ["hidden top"])
        self.assertNotIn("\x02", lines[0])

    def test_assets_become_full_urls(self):
        _, _, targets = text('<p><a href="/wiki-assets/x.png">pic</a></p>')
        self.assertEqual(targets, [BASE + "/wiki-assets/x.png"])

    def test_lists_nest_and_hang(self):
        shown, _, _ = text("<ul><li>one %s</li><li>two<ul><li>inner</li></ul></li></ul><ol start=\"3\">"
                           "<li>three</li></ol>" % ("x " * 50))
        self.assertTrue(shown[0].startswith("• one"))
        self.assertTrue(shown[1].startswith("  x"))
        self.assertIn("  • inner", shown)
        self.assertIn("3. three", shown)

    def test_code_blocks_keep_their_lines_and_cut_long_ones(self):
        shown, lines, _ = text("<pre><code>a\n  b\n%s</code></pre>" % ("y" * 100))
        self.assertEqual(shown[0].rstrip(), " a")
        self.assertEqual(shown[1].rstrip(), "   b")
        self.assertTrue(shown[2].endswith("»"))
        self.assertTrue(all(len(line) == WIDTH for line in shown))
        self.assertTrue(all(line.startswith("\x01p") for line in lines))

    def test_quote_has_a_bar_on_every_line(self):
        shown, _, _ = text("<blockquote><p>one</p><p>two</p></blockquote>")
        self.assertEqual(shown, ["│ one", "│", "│ two"])

    def test_small_table_is_a_grid(self):
        shown, _, _ = text("<table><thead><tr><th>A</th><th>B</th></tr></thead>"
                           "<tbody><tr><td>1</td><td>two</td></tr></tbody></table>")
        self.assertEqual(shown, ["┌───┬─────┐",
                                 "│ A │ B   │",
                                 "├───┼─────┤",
                                 "│ 1 │ two │",
                                 "└───┴─────┘"])

    def test_wide_table_wraps_in_its_cells(self):
        cell = "lots of words in this cell " * 4
        shown, _, _ = text("<table><tr><th>Name</th><th>Text</th></tr><tr><td>x</td><td>%s</td></tr></table>" % cell)
        self.assertTrue(all(len(line) <= WIDTH for line in shown))
        self.assertGreater(len(shown), 5)

    def test_table_with_too_many_columns_becomes_records(self):
        head = "".join("<th>Column%d</th>" % i for i in range(20))
        row = "".join("<td>value%d</td>" % i for i in range(20))
        shown, _, _ = text("<table><tr>%s</tr><tr>%s</tr></table>" % (head, row))
        self.assertIn("Column3: value3", shown)

    def test_images_by_language(self):
        self.assertEqual(text('<p><img src="a.png" alt="Shot"></p>')[0], ["[Image: Shot]"])
        self.assertEqual(text('<p><img src="a.png" alt="Kep"></p>', "hu")[0], ["[Kép: Kep]"])

    def test_control_characters_and_entities(self):
        shown, _, _ = text("<p>a\x01b &amp; &lt;c&gt;</p>")
        self.assertEqual(shown, ["ab & <c>"])

    def test_scripts_are_dropped(self):
        shown, _, _ = text("<p>x</p><script>alert(1)</script><style>p{}</style>")
        self.assertEqual(shown, ["x"])


if __name__ == "__main__":
    unittest.main()
