import glob, os, sys
lib, out = sys.argv[1], sys.argv[2]
# embed.py LIB SALIDA [carpetas a omitir...]   (o: --only nombres...  solo esas carpetas/archivos de lib)
rest = sys.argv[3:]
only = set(rest[1:]) if rest[:1] == ["--only"] else None
skip = set() if only is not None else set(rest)
with open(out, "w") as f:
    f.write("#pragma once\n#include <map>\n#include <string>\n\n")
    f.write("static const std::map<std::string, const char*> EMBEDDED = {\n")
    seen = {}
    for p in sorted(glob.glob(os.path.join(lib, "**", "*.dg"), recursive=True) + glob.glob(os.path.join(lib, "**", "*.css"), recursive=True)):
        rel = os.path.relpath(p, lib).split(os.sep)
        if skip & set(rel[:-1]):
            continue
        if only is not None and rel[0].rsplit(".", 1)[0] not in only and rel[0] not in only:
            continue
        name = os.path.basename(p)[:-3] if p.endswith(".dg") else os.path.basename(p)
        assert name not in seen, "duplicate library name %s: %s and %s" % (name, seen.get(name), p)
        seen[name] = p
        text = open(p, encoding="utf-8").read()
        assert ')DGLIB"' not in text and len(text) < 60000
        f.write('    {"%s", R"DGLIB(%s)DGLIB"},\n' % (name, text))
    f.write("};\n")
