#!/usr/bin/env python3
"""ASCII STL to UVF Converter (indexed geometry with shared vertices)

Updates:
- Switch from unindexed flat position buffer slicing to indexed mesh with a uint32 index buffer.
- Per STL solid Face now references a contiguous range in the global index buffer (startIndex/endIndex are index element offsets, end exclusive, length multiple of 3).
- Vertices are deduplicated by exact (x,y,z) float32 tuple equality to reduce size.

Notes:
- ASCII STL only (binary STL not supported).
- Normals are ignored (could be re-added as a future scalar/attribute section if required).
- Face ranges remain contiguous because triangles are appended in STL traversal order per solid.
"""
from __future__ import annotations
import os, re, json, argparse, sys
from dataclasses import dataclass
from typing import List, Tuple, Optional, Dict
import numpy as np

SOLID_RE = re.compile(r"^\s*solid(?:\s+(.*))?$")
ENDSOLID_RE = re.compile(r"^\s*endsolid(?:\s+.*)?$")
FACET_RE = re.compile(r"^\s*facet\s+normal\s+(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s+(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s+(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s*$")
VERTEX_RE = re.compile(r"^\s*vertex\s+(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s+(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s+(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s*$")
OUTER_LOOP_RE = re.compile(r"^\s*outer\s+loop\s*$")
END_LOOP_RE = re.compile(r"^\s*endloop\s*$")
END_FACET_RE = re.compile(r"^\s*endfacet\s*$")

class STLParseError(RuntimeError):
  pass

@dataclass
class SolidRecord:
  name: str
  start_vertex: int  # inclusive vertex index
  end_vertex: int = 0  # exclusive vertex index
  triangle_count: int = 0
  def finalize(self, current_vertex_count: int):
    self.end_vertex = current_vertex_count
    self.triangle_count = (self.end_vertex - self.start_vertex) // 3


def parse_ascii_stl(path: str) -> Tuple[np.ndarray, List[SolidRecord]]:
  """Parse an ASCII STL file into a flat duplicated vertex array and per-solid records.

  The STL grammar is handled with a simple state machine. Each triangle contributes
  three vertices appended (no deduplication here). Per-solid start/end are recorded
  in units of duplicated vertices (not floats) at this stage.
  """
  duplicated_vertex_scalars: List[float] = []  # flat x,y,z sequence
  solid_records: List[SolidRecord] = []
  current_solid: Optional[SolidRecord] = None
  state = "idle"            # idle | in_solid | in_facet | in_loop
  facet_vertex_count = 0    # number of vertices seen in current facet (expect 3)

  with open(path, 'r', encoding='utf-8', errors='ignore') as stl_file:
    for line_number, raw_line in enumerate(stl_file, 1):
      line = raw_line.rstrip('\n')

      # ---------------- idle state ----------------
      if state == 'idle':
        solid_match = SOLID_RE.match(line)
        if solid_match:
          current_solid = SolidRecord((solid_match.group(1) or '').strip() or 'unnamed', len(duplicated_vertex_scalars)//3)
          solid_records.append(current_solid)
          state = 'in_solid'
        elif line.strip() == '' or line.lstrip().startswith('solid'):
          # Skip blank or stray 'solid' lines
          continue
        else:
          if '\0' in line:
            raise STLParseError('Binary STL not supported (NUL found)')
          continue
        continue

      # ---------------- in_solid state ----------------
      if state == 'in_solid':
        if ENDSOLID_RE.match(line):
          if current_solid:
            current_solid.finalize(len(duplicated_vertex_scalars)//3)
            current_solid = None
          state = 'idle'
          continue
        if FACET_RE.match(line):
          state = 'in_facet'
          facet_vertex_count = 0
          continue
        if SOLID_RE.match(line):  # implicit end of previous solid
          if current_solid:
            current_solid.finalize(len(duplicated_vertex_scalars)//3)
          solid_match = SOLID_RE.match(line)
          current_solid = SolidRecord((solid_match.group(1) or '').strip() or 'unnamed', len(duplicated_vertex_scalars)//3)
          solid_records.append(current_solid)
          state = 'in_solid'
          continue
        continue

      # ---------------- in_facet state ----------------
      if state == 'in_facet':
        if OUTER_LOOP_RE.match(line):
          state = 'in_loop'
          continue
        if VERTEX_RE.match(line):  # some malformed files omit 'outer loop'
          state = 'in_loop'
        else:
          continue

      # ---------------- in_loop state ----------------
      if state == 'in_loop':
        match_vertex = VERTEX_RE.match(line)
        if match_vertex:
          x_val, y_val, z_val = map(float, match_vertex.groups())
          duplicated_vertex_scalars.extend([x_val, y_val, z_val])
          facet_vertex_count += 1
          # Wait for endfacet; just keep counting until 3 vertices collected
          continue
        if END_LOOP_RE.match(line):
          # 'endloop' just signals facet vertex list end; actual facet ends at 'endfacet'
          continue
        if END_FACET_RE.match(line):
            if not current_solid:
              raise STLParseError(f'endfacet outside solid at line {line_number}')
            state = 'in_solid'
            continue
        if ENDSOLID_RE.match(line):
          if current_solid:
            current_solid.finalize(len(duplicated_vertex_scalars)//3)
            current_solid = None
          state = 'idle'
          continue
        continue

  if current_solid is not None:
    current_solid.finalize(len(duplicated_vertex_scalars)//3)

  vertices_array = (np.array(duplicated_vertex_scalars, dtype=np.float32).reshape(-1,3)
                    if duplicated_vertex_scalars else np.empty((0,3), dtype=np.float32))
  return vertices_array, solid_records


def write_positions_binary(vertices: np.ndarray, out_dir: str, base: str):
  resources_dir = os.path.join(out_dir, 'resources', base)
  os.makedirs(resources_dir, exist_ok=True)
  bin_rel = f'resources/{base}/{base}.bin'
  bin_path = os.path.join(resources_dir, f'{base}.bin')
  with open(bin_path, 'wb') as f:
    data = vertices.astype(np.float32).tobytes()
    f.write(data)
  section = {
    'dType': 'float32',
    'dimension': 3,
    'length': vertices.shape[0]*3*4,  # bytes
    'name': 'position',
    'offset': 0
  }
  return [section], bin_rel


def build_manifest(unique_vertices: np.ndarray, indices: np.ndarray, solid_records: List[SolidRecord], buffer_sections, bin_relative_path: str, name: str, output_dir: str) -> str:
  """Assemble UVF manifest structure for indexed geometry.

  startIndex/endIndex of each Face refer to uint32 index buffer element offsets (exclusive end).
  """
  geometry_id = f'{name}-geom'
  face_objects = []
  for face_seq, solid_record in enumerate(solid_records):
    face_id = f'{solid_record.name}-{face_seq}' if solid_record.name != 'unnamed' else f'face-{face_seq}'
    start_index = solid_record.start_vertex
    end_index = solid_record.end_vertex
    segment_length = end_index - start_index
    if segment_length % 3 != 0:
      raise ValueError(f'Face {face_id} index segment length {segment_length} not multiple of 3')
    face_objects.append({
      'attributions': { 'packedParentId': geometry_id },
      'id': face_id,
      'properties': {
        'alpha': 1.0,
        'color': 0xFFFFFF,
        'bufferLocations': {
          'indices': [{
            'bufNum': 0,
            'startIndex': start_index,
            'endIndex': end_index
          }]
        },
        'tags': [f'solid:{solid_record.name}', f'triangles:{solid_record.triangle_count}', 'rangeSemantics:indexElements']
      },
      'type': 'Face'
    })

  manifest = [
    {
      'attributions': { 'members': [geometry_id] },
      'id': 'root_group',
      'properties': {
        'transform': [1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0],
        'type': 0
      },
      'type': 'GeometryGroup'
    },
    {
      'attributions': {
        'edges': [],
        'faces': [fo['id'] for fo in face_objects],
        'vertices': []
      },
      'id': geometry_id,
      'properties': { 'tags': ['geometry:surface', 'source:stl', 'indexed:true'] },
      'resources': {
        'buffers': {
          'path': bin_relative_path,
          'sections': buffer_sections,
          'type': 'buffers'
        }
      },
      'type': 'SolidGeometry'
    },
    *face_objects
  ]
  manifest_path = os.path.join(output_dir, 'manifest.json')
  with open(manifest_path, 'w') as manifest_file:
    json.dump(manifest, manifest_file, indent=2)
  return manifest_path

def build_indexed_geometry_per_solid(duplicated_vertices: np.ndarray, solid_records: List[SolidRecord], deduplicate: bool) -> Tuple[np.ndarray, np.ndarray]:
  """Build indexed geometry with optional per-solid deduplication (no cross-solid merging).

  For each solid we process its vertex slice [start_vertex:end_vertex) in duplicated vertex space (each row is one vertex),
  optionally deduplicate inside that slice only, append unique vertices to a global list and append remapped indices
  preserving original triangle order.

  We then overwrite SolidRecord.start_vertex / end_vertex to reflect index buffer element range (not vertex count anymore).
  """
  if duplicated_vertices.size == 0:
    return duplicated_vertices, np.zeros((0,), dtype=np.uint32)
  global_unique_vertices: List[Tuple[float,float,float]] = []
  global_index_elements: List[int] = []
  original_total_vertices = len(duplicated_vertices)
  for solid_record in solid_records:
    solid_vertex_slice = duplicated_vertices[solid_record.start_vertex:solid_record.end_vertex]
    if not deduplicate:
      base_vertex_offset = len(global_unique_vertices)
      for vertex in solid_vertex_slice.tolist():
        global_unique_vertices.append(tuple(vertex))
      local_index_elements = list(range(base_vertex_offset, base_vertex_offset + solid_vertex_slice.shape[0]))
    else:
      local_vertex_map: Dict[Tuple[float,float,float], int] = {}
      local_unique_vertices: List[Tuple[float,float,float]] = []
      local_index_elements: List[int] = []
      for vertex in solid_vertex_slice.tolist():
        key = tuple(vertex)
        local_id = local_vertex_map.get(key)
        if local_id is None:
          local_id = len(local_unique_vertices)
          local_vertex_map[key] = local_id
          local_unique_vertices.append(key)
        local_index_elements.append(local_id)
      base_vertex_offset = len(global_unique_vertices)
      global_unique_vertices.extend(local_unique_vertices)
      local_index_elements = [base_vertex_offset + li for li in local_index_elements]
    new_start_index = len(global_index_elements)
    global_index_elements.extend(local_index_elements)
    new_end_index = len(global_index_elements)
    solid_record.start_vertex = new_start_index
    solid_record.end_vertex = new_end_index
  unique_vertices_array = np.array(global_unique_vertices, dtype=np.float32)
  indices_array = np.array(global_index_elements, dtype=np.uint32)
  print(
    f'Per-solid dedup: original_vertices={original_total_vertices} unique_vertices={len(unique_vertices_array)} '
    f'reduction={original_total_vertices / max(1, len(unique_vertices_array)):.2f}x'
  )
  return unique_vertices_array, indices_array

def write_indexed_binary(index_buffer: np.ndarray, unique_vertices: np.ndarray, output_dir: str, base_name: str):
  """Write combined index + position binary file and return buffer section metadata."""
  resources_dir = os.path.join(output_dir, 'resources', base_name)
  os.makedirs(resources_dir, exist_ok=True)
  bin_rel_path = f'resources/{base_name}/{base_name}.bin'
  bin_abs_path = os.path.join(resources_dir, f'{base_name}.bin')
  with open(bin_abs_path, 'wb') as binary_file:
    index_bytes = index_buffer.tobytes()
    binary_file.write(index_bytes)
    position_bytes = unique_vertices.astype(np.float32).tobytes()
    binary_file.write(position_bytes)
  buffer_sections = [
    {'dType': 'uint32', 'dimension': 1, 'length': index_buffer.size * 4, 'name': 'indices', 'offset': 0},
    {'dType': 'float32', 'dimension': 3, 'length': unique_vertices.shape[0]*12, 'name': 'position', 'offset': index_buffer.size * 4}
  ]
  return buffer_sections, bin_rel_path


def main():
  p = argparse.ArgumentParser(description='Convert ASCII STL to UVF manifest (indexed with shared vertices)')
  p.add_argument('stl_file')
  p.add_argument('-o','--output', default='stl_output')
  p.add_argument('-n','--name', default=None)
  p.add_argument('--no-dedup', action='store_true', help='Disable per-solid vertex dedup (every vertex unique)')
  args = p.parse_args()

  if not os.path.isfile(args.stl_file):
    print(f'Error: file {args.stl_file} not found', file=sys.stderr)
    return 1
  base_name = args.name or os.path.splitext(os.path.basename(args.stl_file))[0]
  os.makedirs(args.output, exist_ok=True)
  try:
    duplicated_vertices_array, solid_records = parse_ascii_stl(args.stl_file)
    original_vertex_count = len(duplicated_vertices_array)
    triangle_count = original_vertex_count // 3
    print(f'Parsed solids={len(solid_records)} total_vertices={original_vertex_count} triangles={triangle_count}')
    for solid_record in solid_records:
      print(f'  solid {solid_record.name} triangles={solid_record.triangle_count} dupVerts={solid_record.triangle_count*3}')
    if original_vertex_count % 3 != 0:
      raise ValueError('Vertex count not multiple of 3 (invalid triangle mesh)')
    # Build indexed geometry per-solid (no cross-solid merging to avoid range contamination)
    unique_vertices_array, index_buffer = build_indexed_geometry_per_solid(duplicated_vertices_array, solid_records, deduplicate=not args.no_dedup)
    # Validate index buffer size and references
    if index_buffer.size % 3 != 0:
      raise ValueError('Index buffer length not multiple of 3')
    max_index_reference = int(index_buffer.max()) if index_buffer.size else -1
    if max_index_reference >= len(unique_vertices_array):
      raise ValueError(f'Max index {max_index_reference} >= unique vertex count {len(unique_vertices_array)}')
    buffer_sections, bin_rel_path = write_indexed_binary(index_buffer, unique_vertices_array, args.output, base_name)
    manifest_path = build_manifest(unique_vertices_array, index_buffer, solid_records, buffer_sections, bin_rel_path, base_name, args.output)
    print('Done')
    print('Output dir:', os.path.abspath(args.output))
    print('Manifest:', manifest_path)
    return 0
  except STLParseError as e:
    print('STL parse error:', e, file=sys.stderr)
    return 1
  except Exception as e:
    print('Error:', e, file=sys.stderr)
    return 1

if __name__ == '__main__':
  raise SystemExit(main())
