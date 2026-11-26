#!/usr/bin/env python3
"""Converte docs/ do ZSDK em páginas compatíveis com a GitHub Wiki.

docs/ continua sendo a fonte da verdade; a wiki é saída gerada.

Uso:
    python tools/docs2wiki.py --out ../zsdk.wiki [--repo dono/zsdk] [--branch main] [--clean]

O que é feito:
  * docs/README.md        -> Home.md
  * docs/NN_nome.md       -> NN_nome.md   (página "NN_nome")
  * bindings/README.md    -> Bindings.md
  * links entre docs      -> [texto](NomeDaPagina#ancora)
  * links para código     -> URL absoluta github.com/<repo>/blob|tree/<branch>/...
  * gera _Sidebar.md (navegação) e _Footer.md (aviso de página gerada)
"""

import argparse
import os
import posixpath
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Arquivo de origem (relativo à raiz) -> nome da página na wiki.
EXTRA_PAGES = {
    "docs/README.md": "Home",
    "bindings/README.md": "Bindings",
}

LINK_RE = re.compile(r"(!?)\[([^\]]*)\]\(([^)\s]+)(\s+\"[^\"]*\")?\)")
FENCE_RE = re.compile(r"^\s*(```|~~~)")
INLINE_CODE_RE = re.compile(r"(`+)(.+?)\1")


def collect_pages():
    pages = {"docs/README.md": EXTRA_PAGES["docs/README.md"]}
    for f in sorted((ROOT / "docs").glob("*.md")):
        rel = f.relative_to(ROOT).as_posix()
        pages.setdefault(rel, f.stem)
    for src, name in EXTRA_PAGES.items():  # extras (ex.: Bindings) ficam no fim do menu
        pages.setdefault(src, name)
    return {k: v for k, v in pages.items() if (ROOT / k).is_file()}


def detect_repo():
    env = os.environ.get("GITHUB_REPOSITORY")
    if env:
        return env
    try:
        url = subprocess.check_output(
            ["git", "-C", str(ROOT), "remote", "get-url", "origin"], text=True
        ).strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None
    m = re.search(r"github\.com[:/](.+?)(?:\.git)?$", url)
    return m.group(1) if m else None


def rewrite_target(target, src_rel, pages, repo, branch):
    if re.match(r"^[a-z][a-z0-9+.-]*:", target, re.I) or target.startswith("#"):
        return target  # http:, mailto:, âncora local

    path, sep, anchor = target.partition("#")
    resolved = posixpath.normpath(posixpath.join(posixpath.dirname(src_rel), path))
    if resolved.startswith(".."):
        return target  # fora do repositório: deixa como está

    if resolved in pages:
        return pages[resolved] + (sep + anchor if sep else "")

    kind = "tree" if (ROOT / resolved).is_dir() or path.endswith("/") else "blob"
    url = f"https://github.com/{repo}/{kind}/{branch}/{resolved}"
    return url + (sep + anchor if sep else "")


def convert_line(line, src_rel, pages, repo, branch, is_image_raw=True):
    # Protege trechos `inline code` para não reescrever "links" falsos dentro deles.
    stash = []

    def hide(m):
        stash.append(m.group(0))
        return f"\x00{len(stash) - 1}\x00"

    line = INLINE_CODE_RE.sub(hide, line)

    def repl(m):
        bang, text, target, title = m.group(1), m.group(2), m.group(3), m.group(4) or ""
        new = rewrite_target(target, src_rel, pages, repo, branch)
        if bang and new.startswith("https://github.com/") and "/blob/" in new:
            new += "?raw=true"  # imagens precisam do conteúdo bruto
        return f"{bang}[{text}]({new}{title})"

    line = LINK_RE.sub(repl, line)
    return re.sub(r"\x00(\d+)\x00", lambda m: stash[int(m.group(1))], line)


def convert_file(src_rel, pages, repo, branch):
    out, in_fence = [], False
    for line in (ROOT / src_rel).read_text(encoding="utf-8").splitlines():
        if FENCE_RE.match(line):
            in_fence = not in_fence
            out.append(line)
            continue
        out.append(line if in_fence else convert_line(line, src_rel, pages, repo, branch))
    return "\n".join(out) + "\n"


def page_title(src_rel):
    for line in (ROOT / src_rel).read_text(encoding="utf-8").splitlines():
        if line.startswith("# "):
            return line[2:].strip()
    return Path(src_rel).stem


def build_sidebar(pages):
    lines = ["### ZSDK", "", "* [Início](Home)"]
    for src, name in pages.items():
        if name == "Home":
            continue
        title = page_title(src).replace("`", "")
        lines.append(f"* [{title}]({name})")
    return "\n".join(lines) + "\n"


def build_footer(repo, branch):
    return (
        f"_Página gerada automaticamente a partir de "
        f"[`docs/`](https://github.com/{repo}/tree/{branch}/docs) por "
        f"`tools/docs2wiki.py`. Edite os arquivos no repositório, não aqui._\n"
    )


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", required=True, help="pasta do clone de <repo>.wiki.git")
    ap.add_argument("--repo", default=None, help="dono/repo para links de código (padrão: GITHUB_REPOSITORY ou remote origin)")
    ap.add_argument("--branch", default="main")
    ap.add_argument("--clean", action="store_true", help="remove .md antigos da wiki antes de gerar")
    args = ap.parse_args()

    repo = args.repo or detect_repo()
    if not repo:
        sys.exit("erro: não foi possível detectar o repositório; use --repo dono/repo")

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    if args.clean:
        for f in out.glob("*.md"):
            f.unlink()

    pages = collect_pages()
    for src, name in pages.items():
        (out / f"{name}.md").write_text(convert_file(src, pages, repo, args.branch), encoding="utf-8")
        print(f"{src} -> {name}.md")
    (out / "_Sidebar.md").write_text(build_sidebar(pages), encoding="utf-8")
    (out / "_Footer.md").write_text(build_footer(repo, args.branch), encoding="utf-8")
    print(f"_Sidebar.md, _Footer.md  (links de código -> {repo}@{args.branch})")


if __name__ == "__main__":
    main()
