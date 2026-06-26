import os, struct, sys,zlib

def pack_directory(root_dir, output_pak):
    files = []
    base = os.path.abspath(root_dir)

    for cur, _, filenames in os.walk(root_dir):
        for name in filenames:
            full = os.path.join(cur, name)
            rel = os.path.relpath(full, base).replace("\\", "/")
            files.append((rel, full))

    if not files:
        print("No files found to pack")
        return

    pak_info_size = 24  # sizeof(PakInfo) = 8 + 8 + 8

    with open(output_pak, "wb") as f:
        for rel_path, abs_path in files:
            data = open(abs_path, "rb").read()
            compressed_data = zlib.compress(data)
            path_bytes = rel_path.encode('utf-8')

            file_name_size = len(path_bytes)
            file_size = len(compressed_data)
            total_size = file_name_size + file_size + pak_info_size

            # 写入 PakInfo 结构体: FileNameSize(8) + FileSize(8) + totalSize(8)
            f.write(struct.pack("<QQQ", file_name_size, file_size, total_size))

            # 写入文件名（不包含结尾的\0）
            f.write(path_bytes)

            # 写入压缩后的文件数据
            f.write(compressed_data)

        print(f"Packed {len(files)} files into {output_pak}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python bundle_resources_pak.py <input_dir> <output_pak>")
        sys.exit(1)

    pack_directory(sys.argv[1], sys.argv[2])
