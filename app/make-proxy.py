#!/usr/bin/env python3
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

OUT_H = "kdeconnect_proxy.h"
OUT_CPP = "kdeconnect_proxy.cpp"
XML_PATH = Path("/tmp/combined_kdeconnect.xml")

SERVICE = "org.kde.kdeconnect"
DAEMON_PATH = "/modules/kdeconnect"
DAEMON_IFACE = "org.kde.kdeconnect.daemon"
INTROSPECT_IFACE = "org.freedesktop.DBus.Introspectable"
INTROSPECT_METHOD = "Introspect"

DOCTYPE_RE = re.compile(r"<!DOCTYPE[^>]*>", re.DOTALL)

def parse_introspection(xml_text: str) -> ET.Element:
    # Remove DOCTYPE (which may span multiple lines)
    xml_text = re.sub(DOCTYPE_RE, "", xml_text)
    return ET.fromstring(xml_text)

def run_qdbus(args, check=True, capture_output=True):
    cmd = ["qdbus"] + args
    result = subprocess.run(
        cmd,
        check=check,
        capture_output=capture_output,
        text=True,
    )
    return result.stdout


def introspect(path: str) -> str:
    return run_qdbus([SERVICE, path, f"{INTROSPECT_IFACE}.{INTROSPECT_METHOD}"])

def find_sms_capable_device() -> str:
    print("Finding first SMS-capable KDE Connect device...")
    devices_output = run_qdbus([SERVICE, DAEMON_PATH, f"{DAEMON_IFACE}.devices"])
    device_ids = devices_output.split()

    for dev_id in device_ids:
        path = f"{DAEMON_PATH}/devices/{dev_id}/sms"
        try:
            introspect(path)
            print(f"Using device: {dev_id}")
            return dev_id
        except subprocess.CalledProcessError:
            continue

    print("Error: No SMS-capable KDE Connect device found.", file=sys.stderr)
    sys.exit(1)


def build_combined_xml(device_id: str) -> ET.Element:
    print("Introspecting daemon...")
    daemon_xml = parse_introspection(introspect(DAEMON_PATH))

    print("Introspecting device...")
    device_xml = parse_introspection(
        introspect(f"{DAEMON_PATH}/devices/{device_id}")
    )

    print("Introspecting SMS...")
    sms_xml = parse_introspection(
        introspect(f"{DAEMON_PATH}/devices/{device_id}/sms")
    )

    # Telephony is where incoming SMS notifications live now
    telephony_xml = None
    telephony_path = f"{DAEMON_PATH}/devices/{device_id}/telephony"
    try:
        print("Introspecting telephony...")
        telephony_xml = parse_introspection(introspect(telephony_path))
    except subprocess.CalledProcessError:
        print("Warning: telephony interface not available; continuing without it.")

    # Root <node>
    root = ET.Element("node")

    # Add daemon interfaces directly under root
    for child in daemon_xml:
        if child.tag == "interface":
            root.append(child)

    # /devices/<id>
    devices_node = ET.SubElement(root, "node", {"name": "devices"})
    device_node = ET.SubElement(devices_node, "node", {"name": device_id})

    # Add device interfaces and child nodes (except stub sms/telephony nodes)
    for child in device_xml:
        if child.tag == "interface":
            device_node.append(child)
        elif child.tag == "node":
            name = child.attrib.get("name", "")
            # Skip stub nodes; we’ll attach real sms/telephony below
            if name in ("sms", "telephony"):
                continue
            device_node.append(child)

    # Attach real SMS node
    sms_node = ET.SubElement(device_node, "node", {"name": "sms"})
    for child in sms_xml:
        sms_node.append(child)

    # Attach telephony node if present
    if telephony_xml is not None:
        tel_node = ET.SubElement(device_node, "node", {"name": "telephony"})
        for child in telephony_xml:
            tel_node.append(child)

    return root


def write_xml(root: ET.Element, path: Path):
    print(f"Writing combined XML to {path}...")
    tree = ET.ElementTree(root)
    # ElementTree doesn’t write DOCTYPE; qdbusxml2cpp doesn’t need it
    tree.write(path, encoding="utf-8", xml_declaration=True)


def generate_proxies(xml_path: Path):
    print("Generating proxy classes...")
    subprocess.run(
        ["qdbusxml2cpp", "-p", OUT_H + ":" + OUT_CPP, str(xml_path)],
        check=True,
    )

def comment_out_namespace_device(header_path: Path):
    print(f"Commenting out only the first and last lines of 'namespace device' in {header_path}...")

    lines = header_path.read_text().splitlines(keepends=True)

    start_idx = None
    end_idx = None

    # Find the opening line
    for i, line in enumerate(lines):
        if "namespace device" in line and "{" in line:
            start_idx = i
            break

    if start_idx is None:
        print("No 'namespace device {' block found; nothing to fix.")
        return

    # Find the matching closing brace
    brace_depth = 0
    for i in range(start_idx, len(lines)):
        if "{" in lines[i]:
            brace_depth += 1
        if "}" in lines[i]:
            brace_depth -= 1
            if brace_depth == 0:
                end_idx = i
                break

    if end_idx is None:
        print("Malformed namespace block; skipping.")
        return

    # Comment out ONLY the first and last lines
    lines[start_idx] = "// " + lines[start_idx]
    lines[end_idx] = "// " + lines[end_idx]

    header_path.write_text("".join(lines))
    print("Namespace block opening/closing lines commented out.")

def main():
    device_id = find_sms_capable_device()
    root = build_combined_xml(device_id)
    write_xml(root, XML_PATH)
    generate_proxies(XML_PATH)
    comment_out_namespace_device(Path(OUT_H))

    print("Done.")
    print()


if __name__ == "__main__":
    main()
