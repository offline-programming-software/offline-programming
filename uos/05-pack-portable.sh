#!/usr/bin/env bash
# 把「验收通过的运行环境」冻结成自包含部署包：wine 前缀 + 应用 + 启动器 + 注册脚本。
#
# 为什么要这样做：
#   Wine 方案的失败大多来自「环境差异」（wine 版本、前缀里装了哪些原生 DLL、注册表内容）。
#   把整套环境连同应用一起冻结、在每台机器上用同一份，是把"碰运气"变成"可复现"的关键一步，
#   也是在没有 PQKit Linux 版的前提下，最接近「保证运行」的工程手段。
#
# 用法：
#   bash 05-pack-portable.sh [应用目录] [输出目录]
# 前置：02（前缀）与 03（COM 注册）已执行，且 04（验收）通过。
# 产出：
#   <输出目录>/olp-portable/           可直接拷贝的自包含目录
#   <输出目录>/olp-portable.tar.gz     便于分发的压缩包

set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="${1:-$(cd "$HERE/.." && pwd)}"
OUT_DIR="${2:-$PWD/olp-portable}"
BUNDLE="$OUT_DIR/olp-portable"

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
export WINEPREFIX="${WINEPREFIX:-$HOME/.wine-olp}"

if [ -z "$WINE_CMD" ]; then
    echo "未找到 wine 运行器，先执行 02-fix-wineprefix.sh"
    exit 1
fi
if [ ! -d "$WINEPREFIX" ]; then
    echo "前缀不存在：$WINEPREFIX（先执行 02-fix-wineprefix.sh）"
    exit 1
fi
if [ ! -f "$APP_DIR/olp.exe" ]; then
    echo "找不到 $APP_DIR/olp.exe"
    exit 1
fi
if ! "$WINE_CMD" reg query 'HKCR\CLSID\{32087F6B-AABF-420F-AFEB-5B1EAF4D88F2}\InprocServer32' >/dev/null 2>&1; then
    echo "警告：PQPlatformComponent 尚未注册，建议先执行 03-register-pqkit.sh 并通过 04 验收再打包。"
fi

echo "== 1/4 拷贝 wine 前缀（含注册表与已安装的原生 DLL）=="
echo "   来源：$WINEPREFIX"
echo "   目标：$BUNDLE/wineprefix"
mkdir -p "$BUNDLE"
rm -rf "$BUNDLE/wineprefix"
cp -a "$WINEPREFIX" "$BUNDLE/wineprefix"

echo "== 2/4 把应用放进前缀内的 C:\\olp（这样注册路径与本包位置无关）=="
APPDIR_IN_PREFIX="$BUNDLE/wineprefix/drive_c/olp"
mkdir -p "$APPDIR_IN_PREFIX"
cp -a "$APP_DIR/." "$APPDIR_IN_PREFIX/"

echo "== 3/4 生成启动器与注册脚本 =="
cat >"$BUNDLE/run-olp.sh" <<'LAUNCHER'
#!/usr/bin/env bash
# 自包含启动器：使用包内的 wine 前缀运行 C:\olp\olp.exe
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
export WINEPREFIX="$HERE/wineprefix"
export QT_OPENGL="${QT_OPENGL:-software}"     # 三维/界面异常时先走软件 OpenGL
export WINEDEBUG="${WINEDEBUG:--all}"
export LANG="${LANG:-zh_CN.UTF-8}"

WINE=""
for c in "${WINE_BIN:-}" deepin-wine10-stable deepin-wine6-stable deepin-wine8-stable deepin-wine wine64 wine; do
    [ -n "$c" ] || continue
    if command -v "$c" >/dev/null 2>&1; then WINE="$c"; break; fi
done
if [ -z "$WINE" ]; then
    echo "未找到 wine 运行器。请安装：sudo apt install deepin-wine6-stable"
    echo "或指定：WINE_BIN=/usr/bin/wine ./run-olp.sh"
    exit 1
fi

cd "$HERE/wineprefix/drive_c/olp" || exit 1
exec "$WINE" ./olp.exe "$@"
LAUNCHER
chmod +x "$BUNDLE/run-olp.sh"

cat >"$BUNDLE/register-com.sh" <<'REGISTER'
#!/usr/bin/env bash
# 在包内前缀里注册 PQKit 的 COM 组件（用 C:\olp 路径，与本包放在哪里无关）
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
export WINEPREFIX="$HERE/wineprefix"

WINE=""
for c in "${WINE_BIN:-}" deepin-wine10-stable deepin-wine6-stable deepin-wine8-stable deepin-wine wine64 wine; do
    [ -n "$c" ] || continue
    if command -v "$c" >/dev/null 2>&1; then WINE="$c"; break; fi
done
[ -n "$WINE" ] || { echo "未找到 wine 运行器"; exit 1; }

for d in 'C:\olp\PQKit\bin\UPApiBase.dll' \
         'C:\olp\PQKit\bin\UPApiRawData.dll' \
         'C:\olp\PQKit\bin\RobotArt\bin\RPC.dll' \
         'C:\olp\PQKit\bin\RobotArt\bin\RobotArtPQ.dll' ; do
    if "$WINE" regsvr32 /s "$d" >/dev/null 2>&1; then
        echo "  OK   $d"
    else
        echo "  FAIL $d"
    fi
done
"$WINE" reg query 'HKCR\CLSID\{32087F6B-AABF-420F-AFEB-5B1EAF4D88F2}\InprocServer32' 2>&1 | sed 's/^/  /'
REGISTER
chmod +x "$BUNDLE/register-com.sh"

cat >"$BUNDLE/README.txt" <<'NOTE'
olp 自包含运行包（统信 UOS x86_64 + Wine）
=========================================

用法：
  1) 需要系统已安装 wine 运行器（deepin-wine6-stable 或 wine-staging 均可）
     sudo apt install deepin-wine6-stable
  2) 首次使用先注册一次 COM（前缀若已含注册信息可跳过）
     ./register-com.sh
  3) 启动
     ./run-olp.sh

说明：
  * 应用位于 wineprefix/drive_c/olp，注册表里记录的是 C:\olp\...，因此本包可以放在任意路径。
  * wineprefix 是完整前缀（含已安装的 vcrun/gdiplus/msxml6 等原生组件与注册表），
    请整体拷贝、不要只拷贝其中一部分。
  * 若在别的机器上换了 wine 大版本，首次运行可能需要：WINEPREFIX=./wineprefix wineboot -u
  * 排障：WINEDEBUG=+loaddll,+ole,+actctx,+seh ./run-olp.sh > run.log 2>&1
NOTE

echo "== 4/4 打包 =="
tar czf "$OUT_DIR/olp-portable.tar.gz" -C "$OUT_DIR" olp-portable

echo
echo "完成："
echo "  目录：$BUNDLE"
echo "  压缩：$OUT_DIR/olp-portable.tar.gz  ($(du -h "$OUT_DIR/olp-portable.tar.gz" 2>/dev/null | cut -f1))"
echo "  目录大小：$(du -sh "$BUNDLE" 2>/dev/null | cut -f1)"
