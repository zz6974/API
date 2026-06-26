import json, os, sys, zlib


def bundle(resource_dir, out_cpp):
    # 收集所有文件及其相对路径
    files = []
    for root, dirs, filenames in os.walk(resource_dir):
        for filename in filenames:
            full_path = os.path.join(root, filename)
            # 计算相对于resource_dir的相对路径
            relative_path = os.path.relpath(full_path, resource_dir)
            files.append(relative_path)

    # 对文件列表进行排序，保证输出稳定
    files.sort()

    cpp = []

    cpp.append('#include "CCResources.h"')
    cpp.append("")
    cpp.append("namespace CTL {")
    if not files:
        cpp.append("    CTL::Rcc resources[1] = {")
        cpp.append("        {")
        cpp.append('            "",')
        cpp.append("            {}")
        cpp.append("        }")
        cpp.append("    };")
    else:
        cpp.append("    CTL::Rcc resources[] = {")

        for i, f in enumerate(files):
            full_file_path = os.path.join(resource_dir, f)
            data = open(full_file_path, "rb").read()
            compressed_data = zlib.compress(data)

            hex_strs = [f"0x{b:02x}" for b in compressed_data]
            hex_formatted = ",".join(hex_strs)
            safe_path = f.replace("\\", "/")
            comma = "," if i < len(files) - 1 else ""
            cpp.append("        {")
            cpp.append(f'            "{safe_path}",')
            cpp.append(f"            {{{hex_formatted}}}")
            cpp.append(f"        }}{comma}")

        cpp.append("    };")

    cpp.append("")
    cpp.append("    void CTL::Resources::Init() {")
    cpp.append("        std::unique_lock<std::mutex> lock(Resources::get_lock());")
    cpp.append("        const auto m_resources = Resources::getResources();")
    cpp.append("        for (const auto &r : resources) {")
    cpp.append("            m_resources->insert(std::pair<std::string, std::vector<unsigned char>>(r.path, r.data));")


    cpp.append("        }")
    cpp.append("    }")
    cpp.append("}")
    cpp.append("")

    with open(out_cpp, "w") as f:
        f.write("\n".join(cpp))

if __name__ == "__main__":
    bundle(sys.argv[1], sys.argv[2])
