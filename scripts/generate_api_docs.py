#!/usr/bin/env python3
"""Turn Doxygen's XML output into small Markdown fragments, embedded into
hand-written docs/reference/*.md pages via pymdownx.snippets ("--8<--"
includes, already enabled in mkdocs.yml).

Started as a 3-class pilot (Documentation Phase 4: Timeline, Renderer,
Socket); Documentation Phase 5 widened Doxyfile's INPUT to the complete
public Engine API header tree and extended this script to cover the
remaining public surface: Application, Entity, PhysicsSystem, InputManager,
Collision, Protocol, PlayerRegistry, ServerDispatch.

Pipeline this script performs, end to end:
  1. Run `doxygen Doxyfile` (repo root) -> .docs-build/doxygen-xml/*.xml
  2. Parse that XML with the stdlib xml.etree.ElementTree -- no third-party
     dependency.
  3. Emit one clean Markdown fragment per Reference page into
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
  - Overloaded members (e.g. a deleted copy constructor alongside the real
    one) get a disambiguating parameter hint in their heading; every other
    member uses the bare "Class::Member" form the rest of this site already
    searches for.
  - Namespace-scope symbols that share the one `Engine` namespace compound
    across every header (free functions, enums, type aliases, constants,
    and the structs/classes Doxygen also nests under it) are selected by
    which HEADER FILE declared them (Doxygen's own <location file="...">),
    not by hardcoding a name list per page -- a page's own set of symbols is
    still page-specific *data* (which header it reads from), but the
    selection and rendering logic itself is generic across any header.

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
    (e.g. "std::function< double()>", "Timeline &", and -- inside a nested
    function type like std::function<void(InputManager &input)> -- "Type
    &name" with no space after the &) -- tidy those up so the generated
    signature matches how this codebase actually writes C++ ("Type&"/"Type&
    name", never "Type &"/"Type&name")."""
    raw = text_of(type_elem)
    raw = re.sub(r"<\s+", "<", raw)
    raw = re.sub(r"\s+>", ">", raw)
    raw = re.sub(r"\s+&", "&", raw)
    raw = re.sub(r"\s+\*", "*", raw)
    raw = re.sub(r"&(\w)", r"& \1", raw)
    raw = re.sub(r"\*(\w)", r"* \1", raw)
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
    """Returns (prose, param_bullets). Doxygen's @param tags produce a
    <parameterlist> nested inside <detaileddescription> -- a naive full-text
    extraction would flatten "@param entity ... @param deltaTime ..." into
    unreadable run-on prose, so parameter entries are pulled out separately
    and rendered as their own short bullets instead."""
    brief = text_of(memberdef.find("briefdescription"))
    detailed = memberdef.find("detaileddescription")
    prose_parts = []
    bullets = []
    if detailed is not None:
        for para in detailed.findall("para"):
            parameterlist = para.find("parameterlist")
            if parameterlist is not None:
                for item in parameterlist.findall("parameteritem"):
                    name = text_of(item.find("parameternamelist"))
                    description = text_of(item.find("parameterdescription"))
                    bullets.append(f"- `{name}`: {description}")
            else:
                text = text_of(para)
                if text:
                    prose_parts.append(text)
    prose = " ".join(p for p in [brief] + prose_parts if p)
    return prose, bullets


def emit_brief(lines, memberdef):
    """Shared by every emit_*_section function: appends a member's prose (if
    any), then its @param bullets (if any), each followed by a blank line."""
    prose, bullets = member_brief(memberdef)
    if prose:
        lines.append(prose)
        lines.append("")
    if bullets:
        lines.extend(bullets)
        lines.append("")


def emit_member_section(lines, heading, memberdef, name_override=None):
    lines.append(f"### {heading}")
    lines.append("")
    lines.append("```cpp")
    lines.append(render_signature(memberdef, name_override))
    lines.append("```")
    lines.append("")
    emit_brief(lines, memberdef)


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
    emit_brief(lines, enum_memberdef)


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
    emit_brief(lines, compound)


def constructor_heading(class_name, memberdef):
    """Disambiguates overloaded constructors with a short parameter-type hint;
    every other member keeps the bare "Class::Member" heading this site already
    searches for (see docs/reference/*.md's existing convention)."""
    params = memberdef.findall("param")
    if not params:
        return f"{class_name}::{class_name}()"
    type_names = [render_type(p.find("type")) for p in params]
    return f"{class_name}::{class_name}({', '.join(type_names)})"


def emit_class_members(lines, compound, class_name):
    """Emits every public-type then public-func member of a class/struct
    compound, in declaration order -- generic across any class; nothing here
    is specific to one. The constructor (or each overload) and destructor get
    their own recognizable heading; every other member is "Class::Member"."""
    for section_kind in ("public-type", "public-func"):
        for memberdef in compound.findall(f"./sectiondef[@kind='{section_kind}']/memberdef"):
            name = memberdef.findtext("name")
            if name == class_name:
                heading = constructor_heading(class_name, memberdef)
                emit_member_section(lines, heading, memberdef, name_override=class_name)
            elif name == f"~{class_name}":
                emit_member_section(lines, f"{class_name}::~{class_name}", memberdef)
            else:
                emit_member_section(lines, f"{class_name}::{name}", memberdef)


def emit_constant_section(lines, memberdef):
    """Renders a namespace-scope constant (e.g. Engine::MaxPlayers) as a single
    declaration line with its value -- generic for any such constant."""
    name = memberdef.findtext("name")
    type_str = render_type(memberdef.find("type"))
    initializer = (memberdef.findtext("initializer") or "").strip()
    prefix = "constexpr " if memberdef.get("constexpr") == "yes" else ""
    lines.append(f"### {name}")
    lines.append("")
    lines.append("```cpp")
    lines.append(f"{prefix}{type_str} {name} {initializer};")
    lines.append("```")
    lines.append("")
    emit_brief(lines, memberdef)


def _sorted_by_line(memberdefs):
    """Doxygen's own member ordering within a section is an implementation
    detail (Phase 4 already found one Doxygen-version difference in compound
    filenames) -- sort by source line instead of trusting it, so fragment
    order always matches declaration order in the actual header."""
    return sorted(memberdefs, key=lambda m: int(m.find("location").get("line")))


def namespace_members_from_header(ns_compound, section_kind, header_suffix):
    """Namespace-scope members (enum/typedef/var/func) declared in one specific
    header, identified via Doxygen's own <location file="..."> -- generic
    across any header: every header sharing the `Engine` namespace compound
    contributes its members to the same sectiondef, so filtering by source
    file (rather than hardcoding a name list) is what lets a page's fragment
    track its header automatically as symbols are added or removed."""
    matches = [
        memberdef
        for memberdef in ns_compound.findall(f"./sectiondef[@kind='{section_kind}']/memberdef")
        if memberdef.find("location").get("file", "").endswith(header_suffix)
    ]
    return _sorted_by_line(matches)


def structs_from_header(ns_compound, header_suffix):
    """Struct/class compounds nested under the Engine namespace (Doxygen tags
    both under <innerclass>) whose own <location> matches one specific header
    -- resolved via each <innerclass>'s refid, so a page's fragment picks up a
    newly-added struct in its header automatically, the same way
    namespace_members_from_header does for functions/enums/etc."""
    candidates = []
    for inner in ns_compound.findall("innerclass"):
        refid = inner.get("refid")
        if not refid.startswith("struct_"):
            continue
        struct_root = load_xml(f"{refid}.xml")
        location = struct_root.find("compounddef/location")
        if location is not None and location.get("file", "").endswith(header_suffix):
            candidates.append(struct_root)
    return sorted(candidates, key=lambda root: int(root.find("compounddef/location").get("line")))


def generate_simple_class_fragment(qualified_name, output_filename):
    """A fragment that's just one class's own members -- generic, reused for
    every migrated class that doesn't also need namespace-scope siblings
    (structs/enums/free functions) folded in alongside it."""
    class_name = qualified_name.split("::")[-1]
    compound = load_compound(qualified_name, "class").find("compounddef")
    lines = []
    emit_class_members(lines, compound, class_name)
    write_fragment(output_filename, lines)


def generate_free_functions_fragment(header_suffix, output_filename):
    """A fragment that's just the free functions declared in one header, with
    no enclosing class -- generic, reused for Collision (IsColliding) and
    ServerDispatch (HandleRequest/HandleSessionRequest)."""
    ns_compound = load_compound("Engine", "namespace").find("compounddef")
    lines = []
    for memberdef in namespace_members_from_header(ns_compound, "func", header_suffix):
        emit_member_section(lines, memberdef.findtext("name"), memberdef)
    write_fragment(output_filename, lines)


def generate_renderer_fragment():
    ns_compound = load_compound("Engine", "namespace").find("compounddef")
    lines = []

    emit_struct_section(lines, load_compound("Engine::WindowConfig", "struct"))

    for memberdef in namespace_members_from_header(ns_compound, "enum", "Renderer/Renderer.h"):
        emit_enum_section(lines, memberdef.findtext("name"), memberdef)

    for memberdef in namespace_members_from_header(ns_compound, "func", "Renderer/Renderer.h"):
        emit_member_section(lines, memberdef.findtext("name"), memberdef)

    compound = load_compound("Engine::Renderer", "class").find("compounddef")
    emit_class_members(lines, compound, "Renderer")

    write_fragment("renderer-api.md", lines)


def generate_socket_fragment():
    ns_compound = load_compound("Engine", "namespace").find("compounddef")
    lines = []

    for memberdef in namespace_members_from_header(ns_compound, "enum", "Network/Socket.h"):
        emit_enum_section(lines, memberdef.findtext("name"), memberdef)

    compound = load_compound("Engine::Socket", "class").find("compounddef")
    emit_class_members(lines, compound, "Socket")

    write_fragment("socket-api.md", lines)


def generate_entity_fragment():
    lines = []
    emit_struct_section(lines, load_compound("Engine::Color", "struct"))
    compound = load_compound("Engine::Entity", "class").find("compounddef")
    emit_class_members(lines, compound, "Entity")
    write_fragment("entity-api.md", lines)


def generate_protocol_fragment():
    """The stress test: one header contributing a type alias, a constexpr
    constant, two enums, eight structs, and fourteen free functions to the
    shared Engine namespace compound -- every one of them selected generically
    by source header, none hardcoded by name."""
    ns_compound = load_compound("Engine", "namespace").find("compounddef")
    header = "Network/Protocol.h"
    lines = []

    for memberdef in namespace_members_from_header(ns_compound, "typedef", header):
        emit_member_section(lines, memberdef.findtext("name"), memberdef)

    for memberdef in namespace_members_from_header(ns_compound, "var", header):
        emit_constant_section(lines, memberdef)

    for memberdef in namespace_members_from_header(ns_compound, "enum", header):
        emit_enum_section(lines, memberdef.findtext("name"), memberdef)

    for struct_root in structs_from_header(ns_compound, header):
        emit_struct_section(lines, struct_root)

    for memberdef in namespace_members_from_header(ns_compound, "func", header):
        emit_member_section(lines, memberdef.findtext("name"), memberdef)

    write_fragment("protocol-api.md", lines)


def write_fragment(filename, lines):
    FRAGMENTS_DIR.mkdir(parents=True, exist_ok=True)
    content = "\n".join(lines).rstrip() + "\n"
    (FRAGMENTS_DIR / filename).write_text(content)
    print(f"wrote {FRAGMENTS_DIR / filename}")


def main():
    run_doxygen()

    generate_simple_class_fragment("Engine::Timeline", "timeline-api.md")
    generate_renderer_fragment()
    generate_socket_fragment()

    # Documentation Phase 5: remaining public Engine API.
    generate_simple_class_fragment("Engine::Application", "application-api.md")
    generate_entity_fragment()
    generate_simple_class_fragment("Engine::PhysicsSystem", "physics-system-api.md")
    generate_simple_class_fragment("Engine::InputManager", "input-manager-api.md")
    generate_free_functions_fragment("Collision/Collision.h", "collision-api.md")
    generate_protocol_fragment()
    generate_simple_class_fragment("Engine::PlayerRegistry", "player-registry-api.md")
    generate_free_functions_fragment("Network/ServerDispatch.h", "server-dispatch-api.md")


if __name__ == "__main__":
    main()
