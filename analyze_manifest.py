#!/usr/bin/env python3
"""
UVF Manifest Analyzer and Formatter
Analyzes the generated manifest.json file and provides a structured view
"""

import json
import sys
from typing import Dict, List, Any

def analyze_manifest(manifest_path: str):
    """Analyze the manifest.json structure and generate a report"""
    
    try:
        with open(manifest_path, 'r') as f:
            manifest = json.load(f)
    except Exception as e:
        print(f"Error reading manifest: {e}")
        return
    
    print("="*80)
    print("UVF MANIFEST ANALYSIS REPORT")
    print("="*80)
    
    # Count different object types
    geometry_groups = []
    solid_geometries = []
    faces = []
    
    for item in manifest:
        if item['type'] == 'GeometryGroup':
            geometry_groups.append(item)
        elif item['type'] == 'SolidGeometry':
            solid_geometries.append(item)
        elif item['type'] == 'Face':
            faces.append(item)
    
    print(f"\nOVERVIEW:")
    print(f"- Total Objects: {len(manifest)}")
    print(f"- GeometryGroups: {len(geometry_groups)}")
    print(f"- SolidGeometries: {len(solid_geometries)}")
    print(f"- Faces: {len(faces)}")
    
    # Analyze hierarchy
    print(f"\nHIERARCHY STRUCTURE:")
    root_group = next((item for item in manifest if item['id'] == 'root_group'), None)
    
    if root_group:
        print(f"📁 {root_group['id']} (Root)")
        members = root_group.get('attributions', {}).get('members', [])
        
        for member_id in members:
            member = next((item for item in manifest if item['id'] == member_id), None)
            if member:
                print(f"  📁 {member['id']}")
                sub_members = member.get('attributions', {}).get('members', [])
                
                for sub_member_id in sub_members:
                    sub_member = next((item for item in manifest if item['id'] == sub_member_id), None)
                    if sub_member:
                        # Find corresponding face
                        face_id = sub_member.get('attributions', {}).get('faces', [])
                        if face_id:
                            face = next((item for item in manifest if item['id'] == face_id[0]), None)
                            if face:
                                # Get buffer info
                                buffer_info = ""
                                if 'resources' in sub_member and 'buffers' in sub_member['resources']:
                                    sections = sub_member['resources']['buffers'].get('sections', [])
                                    indices_section = next((s for s in sections if s['name'] == 'indices'), None)
                                    if indices_section:
                                        triangles = indices_section['length'] // 12  # uint32 * 3
                                        buffer_info = f" ({triangles:,} triangles)"
                                
                                print(f"    📐 {sub_member['id']}")
                                print(f"      🔹 {face['id']}{buffer_info}")
    
    # Data analysis
    print(f"\nDATA ANALYSIS:")
    total_triangles = 0
    total_vertices = 0
    
    for solid in solid_geometries:
        if 'resources' in solid and 'buffers' in solid['resources']:
            sections = solid['resources']['buffers'].get('sections', [])
            
            indices_section = next((s for s in sections if s['name'] == 'indices'), None)
            position_section = next((s for s in sections if s['name'] == 'position'), None)
            
            if indices_section:
                triangles = indices_section['length'] // 12
                total_triangles += triangles
                
            if position_section:
                vertices = position_section['length'] // 12  # float32 * 3
                total_vertices += vertices
            
            # List scalar fields
            scalar_fields = [s['name'] for s in sections if s['name'] not in ['indices', 'position']]
            if scalar_fields:
                print(f"  {solid['id']}:")
                print(f"    - Triangles: {triangles:,}")
                print(f"    - Vertices: {vertices:,}")
                print(f"    - Scalar Fields: {', '.join(scalar_fields)}")
    
    print(f"\nTOTAL STATISTICS:")
    print(f"- Total Triangles: {total_triangles:,}")
    print(f"- Total Vertices: {total_vertices:,}")
    
    # File size analysis
    print(f"\nFILE STRUCTURE:")
    binary_files = set()
    for solid in solid_geometries:
        if 'resources' in solid and 'buffers' in solid['resources']:
            path = solid['resources']['buffers'].get('path', '')
            if path:
                binary_files.add(path)
    
    for bf in sorted(binary_files):
        print(f"  📁 {bf}")

def pretty_print_manifest(manifest_path: str, output_path: str = None):
    """Pretty print the manifest.json file"""
    
    try:
        with open(manifest_path, 'r') as f:
            manifest = json.load(f)
    except Exception as e:
        print(f"Error reading manifest: {e}")
        return
    
    formatted_json = json.dumps(manifest, indent=2, ensure_ascii=False)
    
    if output_path:
        with open(output_path, 'w') as f:
            f.write(formatted_json)
        print(f"Formatted manifest written to: {output_path}")
    else:
        print(formatted_json)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 analyze_manifest.py <manifest.json> [--format <output.json>]")
        sys.exit(1)
    
    manifest_path = sys.argv[1]
    
    if "--format" in sys.argv:
        format_index = sys.argv.index("--format")
        if format_index + 1 < len(sys.argv):
            output_path = sys.argv[format_index + 1]
            pretty_print_manifest(manifest_path, output_path)
        else:
            pretty_print_manifest(manifest_path)
    else:
        analyze_manifest(manifest_path)
