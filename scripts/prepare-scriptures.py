#!/usr/bin/env python3
"""Build the offline seed from eBible's published VPL files without editing verse text."""
from pathlib import Path
from urllib.request import Request, urlopen
from concurrent.futures import ThreadPoolExecutor
from zipfile import ZipFile
import hashlib
import json
import re
import sqlite3

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets' / 'scriptures'
CACHE = ROOT / 'build' / 'scripture-downloads'
BOOKS = list(zip(
    'GEN EXO LEV NUM DEU JOS JDG RUT 1SA 2SA 1KI 2KI 1CH 2CH EZR NEH EST JOB PSA PRO ECC SNG ISA JER LAM EZK DAN HOS JOL AMO OBA JON MIC NAM HAB ZEP HAG ZEC MAL MAT MRK LUK JHN ACT ROM 1CO 2CO GAL EPH PHP COL 1TH 2TH 1TI 2TI TIT PHM HEB JAS 1PE 2PE 1JN 2JN 3JN JUD REV'.split(),
    'Genesis|Exodus|Leviticus|Numbers|Deuteronomy|Joshua|Judges|Ruth|1 Samuel|2 Samuel|1 Kings|2 Kings|1 Chronicles|2 Chronicles|Ezra|Nehemiah|Esther|Job|Psalms|Proverbs|Ecclesiastes|Song of Songs|Isaiah|Jeremiah|Lamentations|Ezekiel|Daniel|Hosea|Joel|Amos|Obadiah|Jonah|Micah|Nahum|Habakkuk|Zephaniah|Haggai|Zechariah|Malachi|Matthew|Mark|Luke|John|Acts|Romans|1 Corinthians|2 Corinthians|Galatians|Ephesians|Philippians|Colossians|1 Thessalonians|2 Thessalonians|1 Timothy|2 Timothy|Titus|Philemon|Hebrews|James|1 Peter|2 Peter|1 John|2 John|3 John|Jude|Revelation'.split('|')))
TRANSLATIONS = [
    dict(id='engwebp', title='World English Bible', abbreviation='WEB', language='English', copyright='Public domain · eBible.org', license='Public domain', source='https://ebible.org/bible/details.php?id=engwebp'),
    dict(id='tel2017', title='ఇండియన్ రివైజ్డ్ వెర్షన్ తెలుగు', abbreviation='IRV Telugu', language='తెలుగు', copyright='© 2017, 2019 Bridge Connectivity Solutions', license='CC BY-SA 4.0 · https://creativecommons.org/licenses/by-sa/4.0/', source='https://ebible.org/bible/details.php?id=tel2017'),
    dict(id='hin2017', title='इंडियन रिवाइज्ड वर्जन हिंदी', abbreviation='IRV Hindi', language='हिन्दी', copyright='© 2017, 2018, 2019 Bridge Connectivity Solutions', license='CC BY-SA 4.0 · https://creativecommons.org/licenses/by-sa/4.0/', source='https://ebible.org/bible/details.php?id=hin2017'),
]

def obtain(meta):
    code = meta['id']
    path = CACHE / (code + '_vpl.zip')
    if not path.exists():
        request = Request('https://ebible.org/Scriptures/' + path.name, headers={'User-Agent': 'Mozilla/5.0 ChurchDeck-data-preparation'})
        path.write_bytes(urlopen(request, timeout=90).read())
    with ZipFile(path) as archive:
        raw = archive.read(code + '_vpl.txt')
        notice = archive.read(code + '_about.htm')
    (OUT / (code + '-notice.html')).write_bytes(notice)
    rows = []
    for line in raw.decode('utf-8-sig').splitlines():
        if not line.strip():
            continue
        match = re.fullmatch(r'([A-Z0-9]{3}) (\d+):(\d+) (.*)', line)
        if not match:
            raise ValueError(f'Unexpected VPL record in {code}: {line[:80]}')
        book, chapter, verse, content = match.groups()
        book = dict(JOH="JHN", NAH="NAM", EZE="EZK", SOL="SNG", JAM="JAS", JOE="JOL", PHI="PHP", MAR="MRK", **{"1JO":"1JN","2JO":"2JN","3JO":"3JN"}).get(book, book)
        rows.append((code, book, int(chapter), int(verse), content))
    known = {book for book, _ in BOOKS}
    if {row[1] for row in rows} != known or len(rows) < 30000:
        raise ValueError(f'Incomplete/unexpected canon for {code}: {len(rows)} records')
    return meta, rows, dict(id=code, verses=len(rows), sha256=hashlib.sha256(raw).hexdigest(), source=meta['source'])

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    CACHE.mkdir(parents=True, exist_ok=True)
    with ThreadPoolExecutor(max_workers=3) as pool:
        datasets = list(pool.map(obtain, TRANSLATIONS))
    path = OUT / 'content-seed.sqlite'
    if path.exists():
        path.unlink()
    db = sqlite3.connect(path)
    db.executescript('''
      PRAGMA user_version=1;
      CREATE TABLE translations(id TEXT PRIMARY KEY,title TEXT,abbreviation TEXT,language TEXT,copyright TEXT,license TEXT,source TEXT);
      CREATE TABLE books(translation TEXT,code TEXT,ordinal INTEGER,name TEXT,PRIMARY KEY(translation,code));
      CREATE TABLE verses(translation TEXT,book TEXT,chapter INTEGER,verse INTEGER,text TEXT,UNIQUE(translation,book,chapter,verse));
      CREATE INDEX passage_lookup ON verses(translation,book,chapter,verse);
      CREATE VIRTUAL TABLE verses_fts USING fts5(text,content='verses',content_rowid='rowid',tokenize="unicode61 remove_diacritics 2 categories 'L* N* M* Co'");
    ''')
    for meta, rows, manifest in datasets:
        db.execute('INSERT INTO translations VALUES(:id,:title,:abbreviation,:language,:copyright,:license,:source)', meta)
        db.executemany('INSERT INTO books VALUES(?,?,?,?)', [(meta['id'], code, i+1, name) for i, (code, name) in enumerate(BOOKS)])
        db.executemany('INSERT INTO verses VALUES(?,?,?,?,?)', rows)
        print(meta['id'], len(rows), 'verses', flush=True)
    db.execute("INSERT INTO verses_fts(verses_fts) VALUES('rebuild')")
    db.commit()
    assert db.execute('PRAGMA integrity_check').fetchone()[0] == 'ok'
    db.execute('VACUUM')
    db.close()
    (OUT / 'manifest.json').write_text(json.dumps([data[2] for data in datasets], ensure_ascii=False, indent=2) + '\n')

if __name__ == '__main__':
    main()
