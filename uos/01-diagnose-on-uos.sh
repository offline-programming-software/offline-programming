#!/usr/bin/env bash
# 在统信 UOS 上采集 olp.exe 的运行环境与失败日志，生成一份可直接回传的报告。
#
# 用法：
#   bash 01-diagnose-on-uos.sh [应用目录]
# 默认应用目录是本仓库根目录（uos/ 的上一级），报告与原始日志写在当前目录。

set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="${1:-$(cd "$HERE/.." && pwd)}"
REPORT="$PWD/olp-uos-report.txt"
RUNLOG="$PWD/olp-wine-run.log"
EXE="$APP_DIR/olp.exe"

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

{
    echo "===== olp on UOS 诊断报告  $(date '+%F %T') ====="
    echo
    echo "--- 系统 ---"
    uname -a
    (lsb_release -a 2>/dev/null || cat /etc/os-release 2>/dev/null) | sed 's/^/  /'
    echo "  架构: $(uname -m) / dpkg: $(dpkg --print-architecture 2>/dev/null)"
    echo
    echo "--- Wine ---"
    if [ -n "$WINE_CMD" ]; then
        echo "  WINE_CMD=$WINE_CMD"
        "$WINE_CMD" --version 2>&1 | sed 's/^/  /'
    else
        echo "  未找到 wine / deepin-wine。请先安装兼容引擎：sudo apt install deepin-wine6-stable"
    fi
    if command -v winetricks >/dev/null 2>&1; then
        echo "  winetricks: $(winetricks --version 2>&1 | head -1)"
    else
        echo "  winetricks: 未安装"
    fi
    if [ -d "$WINEPREFIX" ]; then
        echo "  WINEPREFIX=$WINEPREFIX (已存在)"
    else
        echo "  WINEPREFIX=$WINEPREFIX (不存在)"
    fi
    echo
    echo "--- 应用目录 $APP_DIR ---"
    ls -l "$APP_DIR" 2>&1 | head -40 | sed 's/^/  /'
    for f in \
        olp.exe platforms/qwindows.dll Qt5Core.dll msvcp140.dll vcruntime140.dll \
        PQKit/bin/UPApiBase.dll PQKit/bin/UPApiRawData.dll \
        PQKit/bin/RobotArt/bin/RPC.dll PQKit/bin/RobotArt/bin/RobotArtPQ.dll ; do
        if [ -e "$APP_DIR/$f" ]; then echo "  OK   $f"; else echo "  MISS $f"; fi
    done
    echo
    echo "--- 随包运行库（字节数） ---"
    for f in msvcp140.dll vcruntime140.dll PQKit/bin/RobotArt/bin/msvcp140.dll; do
        [ -e "$APP_DIR/$f" ] && echo "  $f: $(stat -c %s "$APP_DIR/$f") B"
    done
} >>"$REPORT" 2>&1

if [ -z "$WINE_CMD" ]; then
    echo "没有可用的 wine，诊断结束。报告：$REPORT"
    exit 1
fi
if [ ! -f "$EXE" ]; then
    echo "找不到 $EXE，诊断结束。报告：$REPORT"
    exit 1
fi

{
    echo
    echo "--- 启动测试（最多 20 秒，WINEDEBUG 采样） ---"
} >>"$REPORT"

( cd "$APP_DIR" && WINEDEBUG="${WINEDEBUG:-+actctx,+ole,+loaddll,+seh,+module}" timeout 20 "$WINE_CMD" ./olp.exe ) >"$RUNLOG" 2>&1
rc=$?
{
    echo "  退出码: $rc （124=20 秒内未退出，说明进程活着）"
    echo
    echo "--- 日志关键行（末尾 60 行） ---"
} >>"$REPORT"
grep -nEi 'err:|REGDB_E_CLASSNOTREG|80040154|8007007e|import_dll|Unhandled exception|c0000005|c0000135|actctx|PQKit|CoCreateInstance|not found' "$RUNLOG" 2>/dev/null | tail -60 >>"$REPORT"

echo "报告已生成：$REPORT"
echo "原始日志：  $RUNLOG"
echo "把这两个文件回传即可定位问题。"
