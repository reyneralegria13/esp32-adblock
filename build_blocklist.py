#!/usr/bin/env python3
"""Gera data/block.bin (hashes FNV-1a 32 bits ordenados) para a ESP32."""
import argparse, pathlib, struct, urllib.request

LISTAS_PADRAO = [
    "https://raw.githubusercontent.com/StevenBlack/hosts/master/hosts",
]
IGNORAR = {"localhost", "localhost.localdomain", "local",
           "broadcasthost", "0.0.0.0", "ip6-localhost"}

def fnv1a(texto):
    h = 0x811C9DC5
    for b in texto.encode():
        h ^= b
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h

def extrair(linha):
    linha = linha.split("#")[0].strip().lower()
    if not linha or linha.startswith("!"):
        return None
    if linha.startswith("||"):                 # formato AdBlock ||dominio^
        linha = linha[2:].split("^")[0]
    partes = linha.split()
    if len(partes) >= 2 and partes[0] in ("0.0.0.0", "127.0.0.1", "::"):
        d = partes[1]                          # formato hosts
    else:
        d = partes[0]                          # um dominio por linha
    d = d.strip(".")
    return d if "." in d and d not in IGNORAR else None

ap = argparse.ArgumentParser()
ap.add_argument("urls", nargs="*", help="listas extras (hosts ou 1 dominio/linha)")
ap.add_argument("--max", type=int, default=180000, help="limite de dominios")
args = ap.parse_args()

dominios = set()
for url in args.urls or LISTAS_PADRAO:
    print("Baixando", url)
    txt = urllib.request.urlopen(url, timeout=60).read().decode("utf-8", "ignore")
    for linha in txt.splitlines():
        d = extrair(linha)
        if d:
            dominios.add(d)

hashes = sorted({fnv1a(d) for d in dominios})
if len(hashes) > args.max:
    print(f"Aviso: {len(hashes)} dominios, cortando para {args.max}")
    hashes = hashes[: args.max]

pathlib.Path("data").mkdir(exist_ok=True)
with open("data/block.bin", "wb") as f:
    f.write(struct.pack(f"<{len(hashes)}I", *hashes))
print(f"{len(dominios)} dominios -> data/block.bin ({len(hashes) * 4 // 1024} KB)")

