import struct

def create_ply(filename):
    # A grid of vertices
    grid_size = 10
    vertices = []
    for x in range(grid_size):
        for y in range(grid_size):
            vertices.append((float(x), float(y), 0.0))
    
    # Faces connecting the grid
    faces = []
    for x in range(grid_size - 1):
        for y in range(grid_size - 1):
            i = x * grid_size + y
            faces.append((i, i + 1, i + grid_size))
            faces.append((i + 1, i + grid_size + 1, i + grid_size))

    header = f"""ply
format binary_little_endian 1.0
element vertex {len(vertices)}
property float x
property float y
property float z
element face {len(faces)}
property list uchar int vertex_indices
end_header
"""
    with open(filename, 'wb') as f:
        f.write(header.encode('ascii'))
        for v in vertices:
            f.write(struct.pack('<fff', *v))
        for face in faces:
            f.write(struct.pack('<B', len(face)))
            for v_idx in face:
                f.write(struct.pack('<i', v_idx))

if __name__ == "__main__":
    create_ply("test.ply")
