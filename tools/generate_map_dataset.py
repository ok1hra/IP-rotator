#!/usr/bin/env python3
import json
from pathlib import Path
import struct
import gzip

LAND_IN_PATH = Path('tools/map-data/countries-50m.json')
DXCC_PREFIXES_PATH = Path('tools/map-data/dxcc_prefixes.json')
DXCC_EXTRA_ENTITIES_PATH = Path('tools/map-data/dxcc_extra_entities.json')
ADMIN0_LINES_SHP = Path('tools/map-data/ne/ne_10m_admin_0_boundary_lines_land/ne_10m_admin_0_boundary_lines_land.shp')
ADMIN1_LINES_DBF = Path('tools/map-data/ne/ne_10m_admin_1_states_provinces_lines/ne_10m_admin_1_states_provinces_lines.dbf')
ADMIN1_LINES_SHP = Path('tools/map-data/ne/ne_10m_admin_1_states_provinces_lines/ne_10m_admin_1_states_provinces_lines.shp')
OUT_PATH = Path('tools/map-data/map50_dataset.js')
WEB_BOOTSTRAP_PATH = Path('data/map50.js')
WEB_OUT_GZ_PATH = Path('data/map50.js.gz')
LAND_EPSILON_DEG = 0.10
BORDER_EPSILON_DEG = 0.08
LAND_MIN_POINTS = 5
BORDER_MIN_POINTS = 2
SCALE = 100  # store lat/lon as int(lat*100), int(lon*100)
ADMIN1_KEEP_ADM0 = {"USA", "CAN"}

def load_topo(path):
    obj = json.loads(path.read_text())
    scale = obj['transform']['scale']
    translate = obj['transform']['translate']

    arcs = []
    for arc in obj['arcs']:
        x = 0
        y = 0
        pts = []
        for dx, dy in arc:
            x += dx
            y += dy
            lon = x * scale[0] + translate[0]
            lat = y * scale[1] + translate[1]
            pts.append((lat, lon))
        arcs.append(pts)
    return obj, arcs


land_obj, land_arcs = load_topo(LAND_IN_PATH)
dxcc_prefix_map = json.loads(DXCC_PREFIXES_PATH.read_text())
dxcc_extra_entities = json.loads(DXCC_EXTRA_ENTITIES_PATH.read_text())


def arc_points(idx):
    if idx >= 0:
        return active_arcs[idx]
    return list(reversed(active_arcs[~idx]))


def iter_rings(arcs_spec):
    if not isinstance(arcs_spec, list) or not arcs_spec:
        return
    if isinstance(arcs_spec[0], int):
        yield arcs_spec
    else:
        for child in arcs_spec:
            yield from iter_rings(child)


def stitch_ring(ring_arc_idxs):
    out = []
    for i, arc_idx in enumerate(ring_arc_idxs):
        pts = arc_points(arc_idx)
        out.extend(pts if i == 0 else pts[1:])
    return out


def iter_arc_indices(arcs_spec):
    if not isinstance(arcs_spec, list) or not arcs_spec:
        return
    if isinstance(arcs_spec[0], int):
        for arc_idx in arcs_spec:
            yield arc_idx
    else:
        for child in arcs_spec:
            yield from iter_arc_indices(child)


def simplify_dp(points, epsilon):
    if len(points) < 3:
        return points

    keep = [False] * len(points)
    keep[0] = keep[-1] = True
    stack = [(0, len(points) - 1)]
    eps2 = epsilon * epsilon

    while stack:
        start, end = stack.pop()
        x1, y1 = points[start]
        x2, y2 = points[end]
        dx = x2 - x1
        dy = y2 - y1
        den = dx * dx + dy * dy

        best_idx = -1
        best_dist = 0.0
        for i in range(start + 1, end):
            x, y = points[i]
            if den == 0.0:
                d = (x - x1) ** 2 + (y - y1) ** 2
            else:
                t = ((x - x1) * dx + (y - y1) * dy) / den
                px = x1 + t * dx
                py = y1 + t * dy
                d = (x - px) ** 2 + (y - py) ** 2
            if d > best_dist:
                best_dist = d
                best_idx = i

        if best_idx != -1 and best_dist > eps2:
            keep[best_idx] = True
            stack.append((start, best_idx))
            stack.append((best_idx, end))

    return [p for p, k in zip(points, keep) if k]


def polygon_area(points):
    if len(points) < 3:
        return 0.0
    area = 0.0
    prev_lon = points[0][1]
    unwrapped = []
    for lat, lon in points:
        while lon - prev_lon > 180:
            lon -= 360
        while lon - prev_lon < -180:
            lon += 360
        unwrapped.append((lat, lon))
        prev_lon = lon
    for i in range(len(unwrapped)):
        lat1, lon1 = unwrapped[i]
        lat2, lon2 = unwrapped[(i + 1) % len(unwrapped)]
        area += lon1 * lat2 - lon2 * lat1
    return area * 0.5


def polygon_centroid(points):
    if len(points) < 3:
        lat_avg = sum(lat for lat, _ in points) / max(1, len(points))
        lon_avg = sum(lon for _, lon in points) / max(1, len(points))
        return lat_avg, lon_avg
    prev_lon = points[0][1]
    unwrapped = []
    for lat, lon in points:
        while lon - prev_lon > 180:
            lon -= 360
        while lon - prev_lon < -180:
            lon += 360
        unwrapped.append((lat, lon))
        prev_lon = lon
    area2 = 0.0
    cx = 0.0
    cy = 0.0
    for i in range(len(unwrapped)):
        lat1, lon1 = unwrapped[i]
        lat2, lon2 = unwrapped[(i + 1) % len(unwrapped)]
        cross = lon1 * lat2 - lon2 * lat1
        area2 += cross
        cx += (lon1 + lon2) * cross
        cy += (lat1 + lat2) * cross
    if abs(area2) < 1e-9:
        lat_avg = sum(lat for lat, _ in unwrapped) / len(unwrapped)
        lon_avg = sum(lon for _, lon in unwrapped) / len(unwrapped)
    else:
        lon_avg = cx / (3 * area2)
        lat_avg = cy / (3 * area2)
    while lon_avg > 180:
        lon_avg -= 360
    while lon_avg < -180:
        lon_avg += 360
    return lat_avg, lon_avg


# Land outlines from "land" object
active_arcs = land_arcs
land_lines = []
for geom in land_obj['objects']['land']['geometries']:
    for ring in iter_rings(geom['arcs']):
        land_lines.append(stitch_ring(ring))

def read_dbf_rows(path):
    data = path.read_bytes()
    num_records = struct.unpack('<I', data[4:8])[0]
    header_len = struct.unpack('<H', data[8:10])[0]
    record_len = struct.unpack('<H', data[10:12])[0]
    fields = []
    pos = 32
    while pos < header_len and data[pos] != 0x0D:
        name = data[pos:pos + 11].split(b'\x00', 1)[0].decode('ascii', 'ignore')
        field_len = data[pos + 16]
        fields.append((name, field_len))
        pos += 32
    rows = []
    rec_start = header_len
    for i in range(num_records):
        rec = data[rec_start + i * record_len:rec_start + (i + 1) * record_len]
        if not rec or rec[0] == 0x2A:
            rows.append(None)
            continue
        row = {}
        off = 1
        for name, field_len in fields:
            row[name] = rec[off:off + field_len].decode('utf-8', 'ignore').strip()
            off += field_len
        rows.append(row)
    return rows


def read_polyline_shp(path, keep_record=None):
    data = path.read_bytes()
    out = []
    record_index = 0
    pos = 100  # fixed shapefile header size
    while pos + 8 <= len(data):
        _, rec_len_words = struct.unpack('>2i', data[pos:pos + 8])
        pos += 8
        rec_len = rec_len_words * 2
        rec = data[pos:pos + rec_len]
        pos += rec_len
        keep = keep_record(record_index) if keep_record else True
        record_index += 1
        if not keep:
            continue
        if len(rec) < 4:
            continue
        shape_type = struct.unpack('<i', rec[:4])[0]
        if shape_type == 0:
            continue
        if shape_type not in (3, 13, 23):
            continue
        if len(rec) < 44:
            continue
        num_parts, num_points = struct.unpack('<2i', rec[36:44])
        parts_off = 44
        parts = struct.unpack('<' + 'i' * num_parts, rec[parts_off:parts_off + 4 * num_parts])
        pts_off = parts_off + 4 * num_parts
        points = [struct.unpack('<2d', rec[pts_off + i * 16:pts_off + (i + 1) * 16]) for i in range(num_points)]
        for i, start in enumerate(parts):
            end = parts[i + 1] if i + 1 < num_parts else num_points
            segment = points[start:end]
            if len(segment) >= 2:
                # Shapefile stores lon, lat.
                out.append([(lat, lon) for lon, lat in segment])
    return out


admin1_rows = read_dbf_rows(ADMIN1_LINES_DBF)
country_border_lines = read_polyline_shp(ADMIN0_LINES_SHP) + read_polyline_shp(
    ADMIN1_LINES_SHP,
    keep_record=lambda idx: bool(admin1_rows[idx]) and admin1_rows[idx].get('ADM0_A3') in ADMIN1_KEEP_ADM0,
)

def process(lines, epsilon, min_points):
    out = []
    for line in lines:
        simp = simplify_dp(line, epsilon)
        if len(simp) < min_points:
            continue
        out.append([[int(round(lat * SCALE)), int(round(lon * SCALE))] for lat, lon in simp])
    return out

land_q = process(land_lines, LAND_EPSILON_DEG, LAND_MIN_POINTS)
country_q = process(country_border_lines, BORDER_EPSILON_DEG, BORDER_MIN_POINTS)

dxcc_labels = []
active_arcs = land_arcs
for geom in land_obj['objects']['countries']['geometries']:
    name = geom.get('properties', {}).get('name')
    prefix = dxcc_prefix_map.get(name)
    if not prefix:
        continue
    polygons = []
    arcs_spec = geom.get('arcs', [])
    if geom.get('type') == 'Polygon':
        polygons = [arcs_spec]
    elif geom.get('type') == 'MultiPolygon':
        polygons = arcs_spec
    else:
        continue
    best_ring = None
    best_area = 0.0
    for poly in polygons:
        rings = list(iter_rings(poly))
        if not rings:
            continue
        outer = stitch_ring(rings[0])
        area = abs(polygon_area(outer))
        if area > best_area:
            best_area = area
            best_ring = outer
    if not best_ring or best_area <= 0:
        continue
    lat, lon = polygon_centroid(best_ring)
    dxcc_labels.append([
        int(round(lat * SCALE)),
        int(round(lon * SCALE)),
        prefix,
        int(round(best_area * 100)),
        name
    ])
for entity in dxcc_extra_entities:
    dxcc_labels.append([
        int(round(float(entity['lat']) * SCALE)),
        int(round(float(entity['lon']) * SCALE)),
        str(entity['prefix']),
        int(entity.get('area', 1)),
        str(entity.get('name', entity['prefix']))
    ])
dxcc_labels.sort(key=lambda row: (-row[3], row[2]))

def emit_array(name, lines):
    out = [f"var {name} = ["]
    for line in lines:
        out.append("  " + json.dumps(line, separators=(',', ':')) + ",")
    if len(lines) > 0:
        out[-1] = out[-1].rstrip(",")
    out.append("];")
    return "\n".join(out)

js = (
    f"// generated from countries-50m.json land eps={LAND_EPSILON_DEG} / Natural Earth admin0+admin1 line shapefiles border eps={BORDER_EPSILON_DEG}, scale={SCALE}\n"
    + emit_array("LAND_OUTLINES_Q", land_q)
    + "\n"
    + emit_array("COUNTRY_BORDERS_Q", country_q)
    + "\n"
    + emit_array("DXCC_PREFIX_LABELS_Q", dxcc_labels)
    + "\n"
)
OUT_PATH.write_text(js)

js_c = js.replace("var LAND_OUTLINES_Q =", "var LAND_OUTLINES =")
js_c = js_c.replace("var COUNTRY_BORDERS_Q =", "var COUNTRY_BORDERS =")
js_c = js_c.replace("var DXCC_PREFIX_LABELS_Q =", "var DXCC_PREFIX_LABELS =")
WEB_BOOTSTRAP_PATH.write_text(
    "(function(){\n"
    "  function mapDatasetError(message){\n"
    "    try {\n"
    "      window.dispatchEvent(new CustomEvent('map50-error', { detail: String(message) }));\n"
    "    } catch (err) {\n"
    "      console.error(message);\n"
    "    }\n"
    "  }\n"
    "  fetch('/map50.js.gz').then(function(response){\n"
    "    if(!response.ok){\n"
    "      throw new Error('Map dataset request failed.');\n"
    "    }\n"
    "    return response.arrayBuffer();\n"
    "  }).then(function(buffer){\n"
    "    var bytes = new Uint8Array(buffer);\n"
    "    var isGzip = bytes.length >= 2 && bytes[0] === 0x1f && bytes[1] === 0x8b;\n"
    "    if(!isGzip){\n"
    "      return new TextDecoder('utf-8').decode(bytes);\n"
    "    }\n"
    "    if (typeof DecompressionStream !== 'function') {\n"
    "      throw new Error('Browser does not support gzip map loading.');\n"
    "    }\n"
    "    var stream = new Response(buffer).body.pipeThrough(new DecompressionStream('gzip'));\n"
    "    return new Response(stream).text();\n"
    "  }).then(function(source){\n"
    "    (0, eval)(source);\n"
    "    window.dispatchEvent(new Event('map50-ready'));\n"
    "  }).catch(function(error){\n"
    "    console.error(error);\n"
    "    mapDatasetError(error && error.message ? error.message : error);\n"
    "  });\n"
    "})();\n"
)
WEB_OUT_GZ_PATH.write_bytes(gzip.compress(js_c.encode('utf-8'), compresslevel=9, mtime=0))

print('land_lines', len(land_q), 'country_lines', len(country_q))
print('land_pts', sum(len(x) for x in land_q), 'country_pts', sum(len(x) for x in country_q))
print('dxcc_labels', len(dxcc_labels))
print('dataset_bytes', len(js))
