#!/usr/bin/env python3
"""
Cubase Assets Generator for Hollywood Orchestrator.
Generates:
1. Steinberg Cubase Track Archive XML (.xml) configured with Kontakt 8 VST3 and CSS/CSB/CSW
2. Steinberg Cubase Expression Maps (.expressionmap) with CC58 values
3. Multi-Track Standard MIDI Template (.mid)
"""

import os
import struct
import xml.etree.ElementTree as ET
from xml.dom import minidom

KONTAKT_8_CID = "5653544e694b386b6f6e74616b742038"
KONTAKT_8_PATH = "/Library/Audio/Plug-Ins/VST3/Kontakt 8.vst3"
HO_CID = "abcdef019182faeb4d616e7548647067"
HO_PATH = "/Users/mariosalvucci/Library/Audio/Plug-Ins/VST3/Hollywood Orchestrator.vst3"

def prettify(elem):
    rough = ET.tostring(elem, 'utf-8')
    reparsed = minidom.parseString(rough)
    return reparsed.toprettyxml(indent="   ", encoding="utf-8").decode('utf-8')

# -------------------------------------------------------------
# 1. Expression Maps Generator
# -------------------------------------------------------------
def generate_expression_map(name, slots, filename):
    root = ET.Element("instrumentList")
    obj_inst = ET.SubElement(root, "obj", {"class": "PInstrumentList", "ID": "1"})
    ET.SubElement(obj_inst, "string", {"name": "Name", "value": name, "wide": "true"})
    
    slots_list = ET.SubElement(obj_inst, "list", {"name": "Slots", "type": "obj"})
    obj_id = 10
    for slot in slots:
        slot_obj = ET.SubElement(slots_list, "obj", {"class": "PSoundSlot", "ID": str(obj_id)})
        obj_id += 1
        ET.SubElement(slot_obj, "string", {"name": "Name", "value": slot["name"], "wide": "true"})
        ET.SubElement(slot_obj, "int", {"name": "Status", "value": "176"}) # CC message (0xB0 = 176)
        ET.SubElement(slot_obj, "int", {"name": "Data1", "value": str(slot.get("cc", 58))})
        ET.SubElement(slot_obj, "int", {"name": "Data2", "value": str(slot["value"])})
        if slot.get("remote") is not None:
            ET.SubElement(slot_obj, "int", {"name": "RemoteKey", "value": str(slot["remote"])})
        ET.SubElement(slot_obj, "int", {"name": "Color", "value": str(slot.get("color", 1))})
        ET.SubElement(slot_obj, "int", {"name": "ArticType", "value": "1" if slot.get("direction", False) else "0"})

    content = prettify(root)
    with open(filename, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"Generated expression map: {filename}")

# -------------------------------------------------------------
# 2. Track Archive Generator
# -------------------------------------------------------------
def generate_track_archive(archive_name, tracks, filename):
    root = ET.Element("trackarchive")
    header = ET.SubElement(root, "obj", {"class": "FArchiveHeader", "ID": "1"})
    ET.SubElement(header, "int", {"name": "Version", "value": "1"})
    ET.SubElement(header, "string", {"name": "Product", "value": "Cubase Pro", "wide": "true"})
    ET.SubElement(header, "string", {"name": "Architecture", "value": "mac", "wide": "true"})
    ET.SubElement(header, "string", {"name": "VersionString", "value": "15.0", "wide": "true"})
    
    track_list = ET.SubElement(root, "list", {"name": "Tracks", "type": "obj"})
    obj_id = 100
    for trk in tracks:
        track_obj = ET.SubElement(track_list, "obj", {"class": trk.get("class", "MAudioSynthTrack"), "ID": str(obj_id)})
        obj_id += 1
        
        desc = ET.SubElement(track_obj, "obj", {"class": "MTrackDescription", "ID": str(obj_id)})
        obj_id += 1
        ET.SubElement(desc, "string", {"name": "Name", "value": trk["name"], "wide": "true"})
        ET.SubElement(desc, "int", {"name": "Color", "value": str(trk.get("color", 1))})
        
        params = ET.SubElement(track_obj, "obj", {"class": "MTrackParams", "ID": str(obj_id)})
        obj_id += 1
        ET.SubElement(params, "string", {"name": "TrackType", "value": trk.get("type", "Instrument"), "wide": "true"})
        ET.SubElement(params, "int", {"name": "MidiChannel", "value": str(trk["channel"])})
        ET.SubElement(params, "string", {"name": "MidiInput", "value": trk.get("input", "Hollywood Orchestrator"), "wide": "true"})
        ET.SubElement(params, "string", {"name": "MidiOutput", "value": trk.get("output", "Kontakt 8"), "wide": "true"})
        
        if trk.get("expression_map"):
            ET.SubElement(params, "string", {"name": "ExpressionMap", "value": trk["expression_map"], "wide": "true"})
            
        if "plugin" in trk:
            plug = ET.SubElement(params, "obj", {"class": "VstPluginDescriptor", "ID": str(obj_id)})
            obj_id += 1
            p = trk["plugin"]
            ET.SubElement(plug, "string", {"name": "Name", "value": p["name"], "wide": "true"})
            ET.SubElement(plug, "string", {"name": "ClassID", "value": p["cid"], "wide": "true"})
            ET.SubElement(plug, "string", {"name": "Category", "value": p.get("category", "Audio Module Class"), "wide": "true"})
            ET.SubElement(plug, "string", {"name": "SubCategory", "value": p.get("subcategory", "Instrument"), "wide": "true"})
            ET.SubElement(plug, "string", {"name": "Vendor", "value": p.get("vendor", "Native Instruments"), "wide": "true"})
            ET.SubElement(plug, "string", {"name": "Path", "value": p.get("path", ""), "wide": "true"})

    content = prettify(root)
    with open(filename, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"Generated track archive: {filename}")

# -------------------------------------------------------------
# 3. Standard MIDI Template (.mid)
# -------------------------------------------------------------
def write_var_len(val):
    res = bytearray()
    buf = val & 0x7F
    while (val >> 7) > 0:
        val >>= 7
        buf = (buf << 8) | ((val & 0x7F) | 0x80)
    while True:
        res.append(buf & 0xFF)
        if buf & 0x80:
            buf >>= 8
        else:
            break
    return res

def generate_midi_template(track_configs, filename):
    tracks_bytes = []
    # Conductor Track
    cond = bytearray()
    cond += write_var_len(0) + bytes([0xFF, 0x03, len("Tempo / Master")]) + b"Tempo / Master"
    # Set tempo 120 bpm (500000 microseconds)
    cond += write_var_len(0) + bytes([0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20])
    cond += write_var_len(0) + bytes([0xFF, 0x2F, 0x00])
    tracks_bytes.append(cond)
    
    for cfg in track_configs:
        trk = bytearray()
        name_bytes = cfg["name"].encode('utf-8')
        trk += write_var_len(0) + bytes([0xFF, 0x03, len(name_bytes)]) + name_bytes
        # CC58 initialization default = 0 (Sustain)
        ch = (cfg["channel"] - 1) & 0x0F
        trk += write_var_len(0) + bytes([0xB0 | ch, 58, 0])
        # End of track
        trk += write_var_len(0) + bytes([0xFF, 0x2F, 0x00])
        tracks_bytes.append(trk)
        
    header = bytearray(b"MThd")
    header += struct.pack(">IHHH", 6, 1, len(tracks_bytes), 480)
    
    with open(filename, "wb") as f:
        f.write(header)
        for t in tracks_bytes:
            f.write(b"MTrk")
            f.write(struct.pack(">I", len(t)))
            f.write(t)
    print(f"Generated MIDI template: {filename}")

def main():
    os.makedirs("cubase/expression_maps", exist_ok=True)
    os.makedirs("cubase/templates", exist_ok=True)

    # 1. CSS Expression Map
    css_slots = [
        {"name": "Sustain / Legato", "value": 0, "remote": 24, "direction": True},
        {"name": "Spiccato", "value": 20, "remote": None, "direction": False},
        {"name": "Staccatissimo", "value": 30, "remote": 25, "direction": False},
        {"name": "Staccato", "value": 40, "remote": 26, "direction": False},
        {"name": "Sfz / Accent", "value": 50, "remote": None, "direction": False},
        {"name": "Pizzicato", "value": 60, "remote": 27, "direction": False},
        {"name": "Bartok Pizz", "value": 65, "remote": None, "direction": False},
        {"name": "Col Legno", "value": 70, "remote": 32, "direction": False},
        {"name": "Tremolo", "value": 80, "remote": 29, "direction": True},
        {"name": "Trills", "value": 90, "remote": 30, "direction": True},
        {"name": "Marcato", "value": 100, "remote": 28, "direction": False},
        {"name": "Harmonics", "value": 110, "remote": 31, "direction": True}
    ]
    generate_expression_map("Cinematic Studio Strings (CSS)", css_slots, "cubase/expression_maps/Cinematic_Studio_Strings.expressionmap")

    # 2. CSB Expression Map
    csb_slots = [
        {"name": "Sustain / Legato", "value": 0, "remote": 24, "direction": True},
        {"name": "Repetitions", "value": 10, "remote": 25, "direction": False},
        {"name": "Staccatissimo", "value": 20, "remote": None, "direction": False},
        {"name": "Staccato", "value": 40, "remote": 26, "direction": False},
        {"name": "Rips", "value": 55, "remote": 27, "direction": False},
        {"name": "Double Tongue", "value": 70, "remote": 30, "direction": False},
        {"name": "Muted (Straight)", "value": 85, "remote": 31, "direction": True},
        {"name": "Marcato", "value": 100, "remote": 28, "direction": False},
        {"name": "Flutter Tongue", "value": 115, "remote": 29, "direction": True}
    ]
    generate_expression_map("Cinematic Studio Brass (CSB)", csb_slots, "cubase/expression_maps/Cinematic_Studio_Brass.expressionmap")

    # 3. CSW Expression Map
    csw_slots = [
        {"name": "Sustain / Legato", "value": 0, "remote": 24, "direction": True},
        {"name": "Repetitions", "value": 10, "remote": 25, "direction": False},
        {"name": "Staccatissimo", "value": 20, "remote": None, "direction": False},
        {"name": "Staccato", "value": 40, "remote": 26, "direction": False},
        {"name": "Trills", "value": 60, "remote": 27, "direction": True},
        {"name": "Marcato", "value": 100, "remote": 28, "direction": False},
        {"name": "Flutter Tongue", "value": 115, "remote": 29, "direction": True}
    ]
    generate_expression_map("Cinematic Studio Woodwinds (CSW)", csw_slots, "cubase/expression_maps/Cinematic_Studio_Woodwinds.expressionmap")

    # 4. Orchestral Track Definitions
    kontakt_plug = {
        "name": "Kontakt 8",
        "cid": KONTAKT_8_CID,
        "category": "Audio Module Class",
        "subcategory": "Instrument",
        "vendor": "Native Instruments",
        "path": KONTAKT_8_PATH
    }

    ho_plug = {
        "name": "Hollywood Orchestrator",
        "cid": HO_CID,
        "category": "Audio Module Class",
        "subcategory": "Instrument|MidiEffect",
        "vendor": "Antigravity",
        "path": HO_PATH
    }

    orchestral_tracks = [
        {"name": "CSS - Violins 1", "channel": 1, "color": 2, "expression_map": "Cinematic Studio Strings (CSS)", "plugin": kontakt_plug},
        {"name": "CSS - Violins 2", "channel": 2, "color": 2, "expression_map": "Cinematic Studio Strings (CSS)", "plugin": kontakt_plug},
        {"name": "CSS - Violas", "channel": 3, "color": 3, "expression_map": "Cinematic Studio Strings (CSS)", "plugin": kontakt_plug},
        {"name": "CSS - Cellos", "channel": 4, "color": 4, "expression_map": "Cinematic Studio Strings (CSS)", "plugin": kontakt_plug},
        {"name": "CSS - Double Basses", "channel": 5, "color": 5, "expression_map": "Cinematic Studio Strings (CSS)", "plugin": kontakt_plug},
        {"name": "CSB - Trumpets", "channel": 6, "color": 6, "expression_map": "Cinematic Studio Brass (CSB)", "plugin": kontakt_plug},
        {"name": "CSB - French Horns", "channel": 7, "color": 7, "expression_map": "Cinematic Studio Brass (CSB)", "plugin": kontakt_plug},
        {"name": "CSB - Trombones", "channel": 8, "color": 8, "expression_map": "Cinematic Studio Brass (CSB)", "plugin": kontakt_plug},
        {"name": "CSB - Tuba", "channel": 9, "color": 9, "expression_map": "Cinematic Studio Brass (CSB)", "plugin": kontakt_plug},
        {"name": "CSW - Flutes", "channel": 10, "color": 10, "expression_map": "Cinematic Studio Woodwinds (CSW)", "plugin": kontakt_plug},
        {"name": "CSW - Oboes", "channel": 11, "color": 11, "expression_map": "Cinematic Studio Woodwinds (CSW)", "plugin": kontakt_plug},
        {"name": "CSW - Clarinets", "channel": 12, "color": 12, "expression_map": "Cinematic Studio Woodwinds (CSW)", "plugin": kontakt_plug},
        {"name": "CSW - Bassoons", "channel": 13, "color": 13, "expression_map": "Cinematic Studio Woodwinds (CSW)", "plugin": kontakt_plug},
        {"name": "PERC - Timpani", "channel": 14, "color": 14, "plugin": kontakt_plug},
        {"name": "PERC - Orchestral Perc", "channel": 15, "color": 15, "plugin": kontakt_plug},
        {"name": "HOLLYWOOD ORCHESTRATOR - Controller", "channel": 1, "color": 16, "type": "Instrument", "plugin": ho_plug, "input": "All MIDI Inputs", "output": "Master"}
    ]

    # Individual Instrument Tracks Archive
    generate_track_archive("Hollywood_Orchestrator_Kontakt8_CSS_CSB_CSW", orchestral_tracks, "cubase/Hollywood_Orchestrator_Kontakt8_CSS_CSB_CSW.xml")

    # Multi-Out Rack Variant
    multiout_tracks = [
        {"name": "Kontakt 8 (CSS / CSB / CSW Master)", "channel": 1, "color": 1, "type": "Instrument", "plugin": kontakt_plug, "input": "Hollywood Orchestrator"}
    ]
    for i, t in enumerate(orchestral_tracks[:-1]):
        multiout_tracks.append({
            "name": f"MIDI {t['channel']:02d} - {t['name']}",
            "channel": t["channel"],
            "class": "MMidiTrack",
            "type": "Midi",
            "color": t.get("color", 1),
            "input": "Hollywood Orchestrator",
            "output": "Kontakt 8",
            "expression_map": t.get("expression_map")
        })
    multiout_tracks.append(orchestral_tracks[-1])
    generate_track_archive("Hollywood_Orchestrator_Kontakt8_MultiOut", multiout_tracks, "cubase/Hollywood_Orchestrator_Kontakt8_MultiOut.xml")

    # MIDI File Template
    generate_midi_template(orchestral_tracks, "cubase/templates/Hollywood_Orchestrator_Template.mid")

    print("\nValidating all XML files...")
    for f in [
        "cubase/expression_maps/Cinematic_Studio_Strings.expressionmap",
        "cubase/expression_maps/Cinematic_Studio_Brass.expressionmap",
        "cubase/expression_maps/Cinematic_Studio_Woodwinds.expressionmap",
        "cubase/Hollywood_Orchestrator_Kontakt8_CSS_CSB_CSW.xml",
        "cubase/Hollywood_Orchestrator_Kontakt8_MultiOut.xml"
    ]:
        ET.parse(f)
        print(f" [VALID] {f}")

    print("\nAll Cubase assets successfully generated and verified!")

if __name__ == "__main__":
    main()
