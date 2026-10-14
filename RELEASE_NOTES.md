TTGWIKI – the Teletype Games wiki on DOS: browse, search and print its public pages, or download them all to read offline.

**Install on DOS:** install `TTGWIKI` with DOSINST, or unzip `TTGWIKI-<version>.zip` to the DOS drive (for example `C:\TTGWIKI`) and start it with `TTGWIKI`. It needs a packet driver and mTCP's `DHCP.EXE`, as DOSINST does. For the Hungarian letters set `CODEPAGE=852` in `TTGWIKI.CFG`.

**Install on Windows 95 and later:** install `TTGWIKIW` with DOSINST, or unzip `TTGWIKIW-<version>.zip` (for example to `C:\TTGWIKIW`) and start `TTGWIKIW.EXE`; it uses Windows' own TCP/IP.

**New in 0.2.0:**

- The export moved to the `WIKI` folder of the Teletype Games FTP server: by default `PORT=21` and `PATH=/WIKI` (port 2121 is closed). An installed 0.1.0 has to be updated, and a `TTGWIKI.CFG` with `PORT=2121` changed
- The server side is only the exporter; the FTP server is [infra/ftp-server](https://git.teletypegames.org/infra/ftp-server)

**0.1.0:**

- The contents: the page tree with the dates, search in the titles while typing (case and accents ignored), English and Hungarian (F2)
- The reader: headings, lists, code blocks, quotes and tables in colors; Tab / Shift+Tab select the links, Enter follows them, Backspace goes back
- A page is downloaded when it is read and kept; a new export deletes only the pages that changed; without the server TTGWIKI reads from the disk
- `TTGWIKI SYNC` (F6) downloads every page, `TTGWIKI SHOW route` prints one
- Code page 437 or 852
- The server side: the exporter and the FTP server of the wiki export (`compose.yaml`)
