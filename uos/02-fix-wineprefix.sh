#!/usr/bin/env bash
# 为 olp.exe 准备一个专用 wine 前缀，并补齐运行库 / 中文字体 / GUI 组件。
#
# 用法：
#   export WINEPREFIX=$HOME/.wine-olp        # 可选，默认就是这个
#   bash 02-fix-wineprefix.sh [应用目录]
#
# 首次执行需要联网（winetricks 下载运行库），可能耗时十几分钟。

set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="${1:-$(cd "$HERE/.." && pwd)}"

detect_wine() {
    for c in "${WINE:-}" deepin-wine10-stable deepin-wine6-stable deepin-wine8-stable deepin-wine wine64 wine; do
        [ -n "$c" ] || continue
        if command -v "$c" >/dev/null 2>&1; then
            echo "$c"
            return 0
        fi
    done
    return 1
}

WINE_CMD="$(detect_wine || true)"
if [ -z "$WINE_CMD" ]; then
    echo "未找到 wine / deepin-wine。请先安装：sudo apt install deepin-wine6-stable"
    exit 1
fi

export WINEPREFIX="${WINEPREFIX:-$HOME/.wine-olp}"
export WINEARCH="${WINEARCH:-win64}"

echo "== 初始化前缀 =="
echo "   WINE_CMD   = $WINE_CMD"
echo "   WINEPREFIX = $WINEPREFIX"
echo "   WINEARCH   = $WINEARCH"
[ -d "$WINEPREFIX" ] || "$WINE_CMD" wineboot -u

if command -v winetricks >/dev/null 2>&1; then
    echo "== 安装运行库与组件（PQKit 的 DLL 由 VS2022 编译，必须用新版运行库） =="
    winetricks -q vcrun2022 || winetricks -q vcrun2019 || echo "  ！vcrun 安装失败，可稍后重试"
    winetricks -q gdiplus msxml6 riched20 || echo "  ！部分组件安装失败"
    winetricks -q corefonts cjkfonts || winetricks -q fakechinese || echo "  ！中文字体安装失败"
    echo
    echo "  提示：如果 PQKit 的 Python 组件（pythonnet）报错，需要再装 .NET："
    echo "        winetricks -q dotnet48      # 体积大、耗时长，按需执行"
else
    echo "!! 未安装 winetricks：sudo apt install winetricks"
    echo "   也可以手动把 vcrun2022 / gdiplus / 中文字体装进前缀后再运行本脚本第 3 步"
fi

echo
echo "== 注册 PQKit 的 COM 组件（关键一步） =="
bash "$HERE/03-register-pqkit.sh" "$APP_DIR"

echo
echo "== 完成。运行方式 =="
echo "   cd \"$APP_DIR\" && WINEPREFIX=$WINEPREFIX QT_OPENGL=software $WINE_CMD ./olp.exe"
