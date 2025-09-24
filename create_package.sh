#!/bin/bash

# 简单的 Linux 打包脚本
# 创建可直接运行的 Linux 软件包

set -e

APP_NAME="SensorMonitor"
VERSION="1.0.0"
PACKAGE_DIR="${APP_NAME}-${VERSION}-linux"

echo "🚀 开始创建 Linux 软件包..."

# 创建包目录
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"

# 复制主程序
echo "📦 复制主程序..."
cp build/SensorMonitor "$PACKAGE_DIR/"
chmod +x "$PACKAGE_DIR/SensorMonitor"

# 复制字体文件
echo "📝 复制字体文件..."
mkdir -p "$PACKAGE_DIR/fonts"
cp utils/NotoSansSC-Black.ttf "$PACKAGE_DIR/fonts/"

# 创建启动脚本
echo "🔧 创建启动脚本..."
cat > "$PACKAGE_DIR/run.sh" << 'EOF'
#!/bin/bash

# SensorMonitor 启动脚本
cd "$(dirname "$0")"

# 检查是否需要安装依赖
echo "🔍 检查系统依赖..."

# 运行程序
echo "🚀 启动 SensorMonitor..."
./SensorMonitor "$@"
EOF

chmod +x "$PACKAGE_DIR/run.sh"

# 创建 README
echo "📄 创建使用说明..."
cat > "$PACKAGE_DIR/README.txt" << EOF
SensorMonitor v${VERSION} - Linux 版本
=====================================

📋 系统要求:
- Linux x86_64 系统
- 支持 OpenGL 的显卡驱动

🚀 运行方法:
1. 直接运行: ./SensorMonitor
2. 或使用启动脚本: ./run.sh

🔧 配置:
- 默认连接: 127.0.0.1:5555
- 数据格式: 128 通道 @ 22.5kHz
- 包大小: 4096 字节

📁 文件说明:
- SensorMonitor: 主程序
- run.sh: 启动脚本
- fonts/: 字体文件目录
- README.txt: 本说明文件

如有问题，请检查系统是否安装了必要的图形驱动程序。
EOF

# 创建桌面快捷方式文件（可选）
echo "🖥️  创建桌面快捷方式文件..."
cat > "$PACKAGE_DIR/SensorMonitor.desktop" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=SensorMonitor
Comment=实时传感器数据监控应用
Exec=%k/SensorMonitor
Icon=%k/icon.png
Path=%k
Terminal=false
Categories=Development;Science;Engineering;
StartupNotify=true
EOF

# 创建简单的安装脚本
cat > "$PACKAGE_DIR/install.sh" << 'EOF'
#!/bin/bash

echo "🔧 SensorMonitor 安装脚本"
echo "=========================="

INSTALL_DIR="$HOME/SensorMonitor"

echo "📂 安装到: $INSTALL_DIR"

# 创建安装目录
mkdir -p "$INSTALL_DIR"

# 复制文件
cp -r ./* "$INSTALL_DIR/"

# 创建桌面快捷方式
DESKTOP_FILE="$HOME/Desktop/SensorMonitor.desktop"
sed "s|%k|$INSTALL_DIR|g" SensorMonitor.desktop > "$DESKTOP_FILE"
chmod +x "$DESKTOP_FILE"

echo "✅ 安装完成！"
echo "🚀 你可以："
echo "   1. 双击桌面快捷方式启动"
echo "   2. 或在终端运行: $INSTALL_DIR/SensorMonitor"
EOF

chmod +x "$PACKAGE_DIR/install.sh"

# 打包成 tar.gz
echo "📦 创建压缩包..."
tar -czf "${PACKAGE_DIR}.tar.gz" "$PACKAGE_DIR"

echo ""
echo "✅ Linux 软件包创建完成！"
echo "📁 包目录: $PACKAGE_DIR/"
echo "📦 压缩包: ${PACKAGE_DIR}.tar.gz"
echo ""
echo "🚀 使用方法："
echo "   1. 解压: tar -xzf ${PACKAGE_DIR}.tar.gz"
echo "   2. 进入目录: cd $PACKAGE_DIR"
echo "   3. 运行: ./SensorMonitor"
echo "   4. 或安装: ./install.sh"
echo ""

# 显示文件大小
ls -lh "${PACKAGE_DIR}.tar.gz"
ls -la "$PACKAGE_DIR"