# 基础开发工具链
sudo pacman -S base-devel  # 替代多个 -dev 包（包含 gcc, make, pkgconf 等）

# 具体包转换：
sudo pacman -S openssl        # 替代 libssl3-dev 和 libssl-dev
sudo pacman -S curl           # 替代 libcurl4-openssl-dev（默认使用 OpenSSL）
sudo pacman -S sqlite         # 替代 libsqlite3-dev
sudo pacman -S glibc          # 替代 libc6-dev（通常已包含在 base-devel 中）
sudo pacman -S glfw-x11       # 替代 libglfw3-dev（或 glfw-wayland）
sudo pacman -S plasma-wayland # 替代 plasma-workspace-wayland（Wayland 会话支持）
sudo pacman -S libxkbcommon   # 替代 libxkbcommon-dev
sudo pacman -S libx11         # 替代 libx11-dev
sudo pacman -S libxinerama    # 替代 libxinerama-dev
sudo pacman -S libxcursor     # 替代 libxcursor-dev
sudo pacman -S libxi          # 替代 libxi-dev
sudo pacman -S webkit2gtk-4.1 # 替代 libwebkit2gtk-4.0-dev（版本号可能不同）
sudo pacman -S wmctrl         # 名称相同
sudo pacman -S openmotif      # 替代 libmotif-dev（或从 AUR 安装 lesstif）
sudo pacman -S libappindicator-gtk3  # 替代 libappindicator3-dev
sudo pacman -S libayatana-appindicator3-1  # 替代 llibayatana-appindicator3-1
