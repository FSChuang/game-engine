#!/usr/bin/env python3
"""Documentation Phase 4 pilot: turn Doxygen's XML output into small Markdown
fragments, embedded into hand-written docs/reference/*.md pages via
pymdownx.snippets ("--8<--" includes, already enabled in mkdocs.yml).

Pipeline this script performs, end to end:
  1. Run `doxygen Doxyfile` (repo root) -> .docs-build/doxygen-xml/*.xml
  2. Parse the XML for exactly the three pilot symbols (Timeline, Renderer,
     Socket) using the stdlib xml.etree.ElementTree -- no third-party
     dependency.
  3. Emit one clean Markdown fragment per pilot page into
     .docs-build/api-fragments/*.md.

Deliberate scope decisions:
  - Only a symbol's *brief*/*detailed* description text and its bare
    signature are extracted. Class-level "detailed description" blocks are
    never emitted here -- those are exactly the kind of semantics/mental-model
    content this project keeps as hand-written Markdown (see
    docs/concepts/*.md, docs/systems/*.md). Duplicating them here would
    recreate the "second hand-written description drifting out of sync"
    problem this whole phase exists to remove.
  - Private members are already excluded by Doxygen itself (EXTRACT_PRIVATE=NO
    in the Doxyfile) -- this script never has private-member XML to read in
    the first place.
  - Overloaded members (Timeline's three constructors) get a disambiguating
    parameter hint in their heading; every other member uses the bare
    "Class::Member" form the rest of this site already searches for.

Usage:
    python3 scripts/generate_api_docs.py

Exits non-zero (and prints Doxygen's own stderr) if Doxygen itself fails, so
a CI step chaining this with `&&` correctly fails the build on a broken
header comment or missing symbol.
"""

import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DOXYFILE = REPO_ROOT / "Doxyfile"
XML_DIR = REPO_ROOT / ".docs-build" / "doxygen-xml"
FRAGMENTS_DIR = REPO_ROOT / ".docs-build" / "api-fragments"


def run_doxygen():
    result = subprocess.run(["doxygen", str(DOXYFILE)], cwd=REPO_ROOT, capture_output=True, text=True)
    sys.stdout.write(result.stdout)
    sys.stderr.write(result.stderr)
    if result.returncode != 0:
        sys.exit(f"doxygen exited with status {result.returncode}")


def load_xml(filename):
    return ET.parse(XML_DIR / filename).getroot()


_COMPOUND_REFIDS = None


def compound_refid(qualified_name, kind):
    """Resolves a compound's XML filename via index.xml's own name->refid table,
    rather than guessing Doxygen's filename-encoding scheme directly (e.g.
    "class_engine_1_1_timeline.xml") -- that encoding is a Doxygen-version
    implementation detail, not a stable contract; different Doxygen versions
    (e.g. Homebrew's bottle vs. Ubuntu's apt package, as CI caught) have used
    different schemes for it. index.xml is Doxygen's own documented lookup
    table and is stable across versions."""
    global _COMPOUND_REFIDS
    if _COMPOUND_REFIDS is None:
        index_root = load_xml("index.xml")
        _COMPOUND_REFIDS = {
            (compound.findtext("name"), compound.get("kind")): compound.get("refid")
            for compound in index_root.findall("compound")
        }
    refid = _COMPOUND_REFIDS.get((qualified_name, kind))
    if refid is None:
        sys.exit(f"generate_api_docs.py: no {kind} named {qualified_name!r} found in Doxygen's index.xml")
    return refid


def load_compound(qualified_name, kind):
    return load_xml(f"{compound_refid(qualified_name, kind)}.xml")


def text_of(elem):
    """Flatten a Doxygen description element (<briefdescription>/<detaileddescription>,
    or a <type> that may contain nested <ref> tags) into plain text. Doxygen wraps
    prose in <para>; refs/computeroutput are inline tags whose .text/.tail must be
    stitched back in, since ElementTree's own itertext() already handles that -- this
    just trims the surrounding whitespace Doxygen's pretty-printed XML adds."""
    if elem is None:
        return ""
    parts = "".join(elem.itertext())
    return " ".join(parts.split())


def render_type(type_elem):
    """Doxygen's own XML renders template/reference types with stray spaces
    (e.g. "std::function< double()>", "Timeline &") -- tidy those up so the
    generated signature matches how this codebase actually writes C++."""
    raw = text_of(type_elem)
    raw = re.sub(r"<\s+", "<", raw)
    raw = re.sub(r"\s+>", ">", raw)
    raw = re.sub(r"\s+&", "&", raw)
    raw = re.sub(r"\s+\*", "*", raw)
    return raw


def render_params(memberdef):
    params = []
    for param in memberdef.findall("param"):
        type_str = render_type(param.find("type"))
        name = param.findtext("declname") or ""
        params.append(f"{type_str} {name}".strip() if name else type_str)
    return ", ".join(params)


def render_signature(memberdef, name_override=None):
    kind = memberdef.get("kind")
    name = name_override or memberdef.findtext("name")
    is_const = memberdef.get("const") == "yes"
    is_explicit = memberdef.get("explicit") == "yes"
    return_type = render_type(memberdef.find("type"))

    if kind == "typedef":
        return f"using {name} = {return_type};"

    # Doxygen's own argsstring carries a trailing "=delete"/"=default" marker
    # for special members declared that way -- surface it explicitly rather
    # than silently rendering a deleted copy constructor as if it were usable.
    argsstring = memberdef.findtext("argsstring") or ""
    if argsstring.endswith("=delete"):
        trailer = " = delete;"
    elif argsstring.endswith("=default"):
        trailer = " = default;"
    else:
        trailer = ";"

    params = render_params(memberdef)
    prefix = "explicit " if is_explicit else ""
    suffix = " const" if is_const else ""
    if return_type:
        return f"{prefix}{return_type} {name}({params}){suffix}{trailer}"
    return f"{prefix}{name}({params}){suffix}{trailer}"


def member_brief(memberdef):
    brief = text_of(memberdef.find("briefdescription"))
    detailed = text_of(memberdef.find("detaileddescription"))
    return " ".join(p for p in (brief, detailed) if p)


def emit_member_section(lines, heading, memberdef, name_override=None):
    lines.append(f"### {heading}")
    lines.append("")
    lines.append("```cpp")
    lines.append(render_signature(memberdef, name_override))
    lines.append("```")
    lines.append("")
    brief = member_brief(memberdef)
    if brief:
        lines.append(brief)
        lines.append("")


def emit_enum_section(lines, class_name, enum_memberdef):
    enum_name = enum_memberdef.findtext("name")
    lines.append(f"### {class_name}")
    lines.append("")
    lines.append("```cpp")
    lines.append(f"enum class {enum_name}")
    lines.append("{")
    values = enum_memberdef.findall("enumvalue")
    for index, value in enumerate(values):
        value_name = value.findtext("name")
        brief = text_of(value.find("briefdescription"))
        comma = "," if index < len(values) - 1 else ""
        comment = f" // {brief}" if brief else ""
        lines.append(f"\t{value_name}{comma}{comment}")
    lines.append("};")
    lines.append("```")
    lines.append("")
    brief = member_brief(enum_memberdef)
    if brief:
        lines.append(brief)
        lines.append("")


def emit_struct_section(lines, struct_root):
    compound = struct_root.find("compounddef")
    name = compound.findtext("compoundname").split("::")[-1]
    lines.append(f"### {name}")
    lines.append("")
    lines.append("```cpp")
    lines.append(f"struct {name}")
    lines.append("{")
    for memberdef in compound.findall("./sectiondef[@kind='public-attrib']/memberdef"):
        field_type = render_type(memberdef.find("type"))
        field_name = memberdef.findtext("name")
        lines.append(f"\t{field_type} {field_name};")
    lines.append("};")
    lines.append("```")
    lines.append("")
    brief = member_brief(compound)
    if brief:
        lines.append(brief)
        lines.append("")


def constructor_heading(class_name, memberdef):
    """Disambiguates overloaded constructors with a short parameter-type hint;
    every other member keeps the bare "Class::Member" heading this site already
    searches for (see docs/reference/*.md's existing convention)."""
    params = memberdef.findall("param")
    if not params:
        return f"{class_name}::{class_name}()"
    type_names = [render_type(p.find("type")) for p in params]
    return f"{class_name}::{class_name}({', '.join(type_names)})"


def generate_timeline_fragment():
    root = load_compound("Engine::Timeline", "class")
    compound = root.find("compounddef")
    lines = []

    # AnchorSource (public-type)
    for memberdef in compound.findall("./sectiondef[@kind='public-type']/memberdef"):
        emit_member_section(lines, "Timeline::AnchorSource", memberdef)

    # Constructors + methods (public-func), in declaration order
    for memberdef in compound.findall("./sectiondef[@kind='public-func']/memberdef"):
        name = memberdef.findtext("name")
        if name == "Timeline":
            heading = constructor_heading("Timeline", memberdef)
            emit_member_section(lines, heading, memberdef, name_override="Timeline")
        else:
            emit_member_section(lines, f"Timeline::{name}", memberdef)

    write_fragment("timeline-api.md", lines)


def generate_renderer_fragment():
    ns_root = load_compound("Engine", "namespace")
    ns_compound = ns_root.find("compounddef")
    lines = []

    # WindowConfig struct
    emit_struct_section(lines, load_compound("Engine::WindowConfig", "struct"))

    # ScalingMode enum
    for memberdef in ns_compound.findall("./sectiondef[@kind='enum']/memberdef"):
        if memberdef.findtext("name") == "ScalingMode":
            emit_enum_section(lines, "ScalingMode", memberdef)

    # ApplyScalingMode / NextScalingMode free functions
    for memberdef in ns_compound.findall("./sectiondef[@kind='func']/memberdef"):
        name = memberdef.findtext("name")
        if name in ("ApplyScalingMode", "NextScalingMode"):
            emit_member_section(lines, name, memberdef)

    # Renderer class
    root = load_compound("Engine::Renderer", "class")
    compound = root.find("compounddef")
    for memberdef in compound.findall("./sectiondef[@kind='public-func']/memberdef"):
        name = memberdef.findtext("name")
        if name == "Renderer":
            heading = constructor_heading("Renderer", memberdef)
            emit_member_section(lines, heading, memberdef, name_override="Renderer")
        elif name == "~Renderer":
            emit_member_section(lines, "Renderer::~Renderer", memberdef)
        else:
            emit_member_section(lines, f"Renderer::{name}", memberdef)

    write_fragment("renderer-api.md", lines)


def generate_socket_fragment():
    ns_root = load_compound("Engine", "namespace")
    ns_compound = ns_root.find("compounddef")
    lines = []

    # SocketRole enum
    for memberdef in ns_compound.findall("./sectiondef[@kind='enum']/memberdef"):
        if memberdef.findtext("name") == "SocketRole":
            emit_enum_section(lines, "SocketRole", memberdef)

    # Socket class
    root = load_compound("Engine::Socket", "class")
    compound = root.find("compounddef")
    for memberdef in compound.findall("./sectiondef[@kind='public-func']/memberdef"):
        name = memberdef.findtext("name")
        if name == "Socket":
            heading = constructor_heading("Socket", memberdef)
            emit_member_section(lines, heading, memberdef, name_override="Socket")
        else:
            emit_member_section(lines, f"Socket::{name}", memberdef)

    write_fragment("socket-api.md", lines)


def write_fragment(filename, lines):
    FRAGMENTS_DIR.mkdir(parents=True, exist_ok=True)
    content = "\n".join(lines).rstrip() + "\n"
    (FRAGMENTS_DIR / filename).write_text(content)
    print(f"wrote {FRAGMENTS_DIR / filename}")


def main():
    run_doxygen()
    generate_timeline_fragment()
    generate_renderer_fragment()
    generate_socket_fragment()


if __name__ == "__main__":
    main()
