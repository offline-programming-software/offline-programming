#!/usr/bin/env bash
# 把 PQKit 的 COM 组件注册进 wine 前缀（修复「PQKit 初始化失败 / 0x80040154」）。
#
# 背景：olp.exe 依靠内嵌的 PQKit.manifest 使用「免注册 COM」创建 PQPlatformComponent，
#       而 wine 对清单里 comClass 的支持不完整，CoCreateInstance 常返回
#       0x80040154 (REGDB_E_CLASSNOTREG)，程序只弹出「PQKit 初始化失败」对话框。
#       这四个 DLL 都导出了 DllRegisterServer/DllGetClassObject，显式注册即可绕过该限制。
#
# 用法：
#   bash 03-register-pqkit.sh [应用目录]

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

# '/' 转 '\'
to_win() { printf '%s' "$1" | sed 's|/|\\|g'; }

echo "== regsvr32 注册 PQKit COM 服务器 =="
echo "   WINEPREFIX = $WINEPREFIX"
echo "   应用目录   = $APP_DIR"

fail=0
for d in \
    "PQKit/bin/UPApiBase.dll" \
    "PQKit/bin/UPApiRawData.dll" \
    "PQKit/bin/RobotArt/bin/RPC.dll" \
    "PQKit/bin/RobotArt/bin/RobotArtPQ.dll" ; do
    if [ ! -f "$APP_DIR/$d" ]; then
        echo "  跳过（文件不存在）: $d"
        continue
    fi
    if "$WINE_CMD" regsvr32 /s "$(to_win "$APP_DIR/$d")" >/dev/null 2>&1; then
        echo "  OK   $d"
    else
        echo "  FAIL $d"
        fail=$((fail + 1))
    fi
done

if [ "$fail" -gt 0 ]; then
    echo
    echo "== 有失败项，回退为注册表导入（PQKit.manifest 里的 94 个 COM 类 + 2 个类型库） =="
    REG="$PWD/PQKit-com-reg.reg"
    template="$(cat "$HERE/PQKit-com-reg.template.reg")"
    # 注意：bash 的 ${var//pat/rep} 会把替换文本里的 \\ 折叠成一个 \，
    # 所以这里先用「含 / 的 Z: 路径」替换占位符，再用 sed 把所有 / 变成 .reg 需要的双反斜杠。
    WINPATH_FWD="Z:$APP_DIR"
    printf '%s\n' "${template//__PQKIT_ROOT__/$WINPATH_FWD}" | sed 's|/|\\\\|g' >"$REG"
    "$WINE_CMD" regedit "$(to_win "$REG")" 2>&1 | tail -3
    echo "  已导入：$REG"
fi

echo
echo "== 校验关键 CLSID（PQPlatformComponent） =="
"$WINE_CMD" reg query 'HKCR\CLSID\{32087F6B-AABF-420F-AFEB-5B1EAF4D88F2}\InprocServer32' 2>&1 | sed 's/^/  /'
