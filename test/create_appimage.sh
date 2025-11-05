#!/bin/bash

# SensorMonitor AppImage 打包脚本
# 使用此脚本将 SensorMonitor 打包为可直接运行的 AppImage

set -e

APP_NAME="SensorMonitor"
VERSION="1.0.0"
ARCH="x86_64"

# 创建 AppDir 结构
APPDIR="${APP_NAME}.AppDir"
echo "创建 AppDir 结构..."

# 清理之前的构建
rm -rf "$APPDIR"
mkdir -p "$APPDIR"/{usr/bin,usr/lib,usr/share/fonts,usr/share/applications,usr/share/icons/hicolor/256x256/apps}

# 复制主程序
echo "复制主程序..."
cp build/SensorMonitor "$APPDIR/usr/bin/"

# 复制字体文件
echo "复制字体文件..."
cp utils/NotoSansSC-Black.ttf "$APPDIR/usr/share/fonts/"

# 创建桌面文件
echo "创建桌面文件..."
cat > "$APPDIR/usr/share/applications/${APP_NAME}.desktop" << EOF
[Desktop Entry]
Type=Application
Name=SensorMonitor
Comment=Real-time sensor data monitoring application
Icon=${APP_NAME}
Exec=${APP_NAME}
Categories=Development;Science;
Terminal=false
EOF

# 创建 AppRun 脚本
echo "创建 AppRun 脚本..."
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash

# 获取 AppImage 目录
HERE="$(dirname "$(readlink -f "${0}")")"

# 设置库路径
export LD_LIBRARY_PATH="$HERE/usr/lib:$HERE/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"

# 设置字体路径
export FONTCONFIG_PATH="$HERE/usr/share/fonts"

# 进入应用目录
cd "$HERE"

# 运行程序
exec "$HERE/usr/bin/SensorMonitor" "$@"
EOF

chmod +x "$APPDIR/AppRun"

# 创建符号链接
echo "创建符号链接..."
ln -sf usr/share/applications/${APP_NAME}.desktop "$APPDIR/${APP_NAME}.desktop"
ln -sf usr/bin/${APP_NAME} "$APPDIR/${APP_NAME}"

# 创建简单图标（如果没有的话）
echo "创建图标..."
cat > "$APPDIR/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png" << 'EOF'
iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==
EOF

# 复制图标到根目录
cp "$APPDIR/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png" "$APPDIR/"

echo "AppDir 结构创建完成。"

# 下载 appimagetool
echo "下载 appimagetool..."
APPIMAGETOOL="appimagetool-${ARCH}.AppImage"
if [ ! -f "$APPIMAGETOOL" ]; then
    wget -q "https://github.com/AppImage/AppImageKit/releases/download/continuous/${APPIMAGETOOL}"
    chmod +x "$APPIMAGETOOL"
fi

# 创建 AppImage
echo "创建 AppImage..."
ARCH=$ARCH ./$APPIMAGETOOL "$APPDIR" "${APP_NAME}-${VERSION}-${ARCH}.AppImage"

echo "✅ AppImage 创建成功: ${APP_NAME}-${VERSION}-${ARCH}.AppImage"
echo "📁 你可以直接运行: ./${APP_NAME}-${VERSION}-${ARCH}.AppImage"
echo "📦 也可以将此文件移动到任何 Linux 系统直接运行"

# 显示文件信息
ls -lh "${APP_NAME}-${VERSION}-${ARCH}.AppImage"