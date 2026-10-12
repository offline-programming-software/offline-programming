#!/usr/bin/env bash
# olp.exe 在统信 UOS 上的「可运行性验收」：启动一次并逐项检查，输出 PASS/FAIL 与下一步建议。
#
# 用法：
#   bash 04-verify-on-uos.sh [应用目录]
# 前置：
#   bash 02-fix-wineprefix.sh .      # 前缀 + 运行库
#   bash 03-register-pqkit.sh .      # PQKit COM 注册
# 产出：
#   olp-acceptance-report.txt        # PASS/FAIL 结论
#   olp-acceptance-run.log           # 本次启动的完整日志（WINEDEBUG 采样）
#
# 环境变量：
#   WAIT_SECS=25                     启动后观察秒数
#   WINEPREFIX=$HOME/.wine-olp

set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="${1:-$(cd "$HERE/.." && pwd)}"
REPORT="$PWD/olp-acceptance-report.txt"
RUNLOG="$PWD/olp-acceptance-run.log"
WAIT_SECS="${WAIT_SECS:-25}"

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

: >"$REPORT"
say() { printf '%s\n' "$*" | tee -a "$REPORT"; }
pass=0
fail=0
ok() { say "  [PASS] $1"; pass=$((pass + 1)); }
no() { say "  [FAIL] $1"; say "         下一步：$2"; fail=$((fail + 1)); }

say "===== olp on UOS 可运行性验收  $(date '+%F %T') ====="
say "APP_DIR    = $APP_DIR"
say "WINEPREFIX = $WINEPREFIX"
say

if [ -z "$WINE_CMD" ]; then
    say "  [FAIL] 未找到 wine 运行器"
    say "         下一步：sudo apt install deepin-wine6-stable（或安装 wine-staging 后 export WINE=wine）"
    exit 1
fi
ok "wine 运行器：$WINE_CMD （$("$WINE_CMD" --version 2>&1 | head -1)）"

for f in olp.exe platforms/qwindows.dll Qt5Core.dll msvcp140.dll vcruntime140.dll \
         PQKit/bin/UPApiBase.dll PQKit/bin/UPApiRawData.dll \
         PQKit/bin/RobotArt/bin/RPC.dll PQKit/bin/RobotArt/bin/RobotArtPQ.dll ; do
    if [ -e "$APP_DIR/$f" ]; then
        ok "文件存在 $f"
    else
        no "缺少 $f" "部署包不完整；PQKit 必须与 olp.exe 同目录且不可改名"
    fi
done

if "$WINE_CMD" reg query 'HKCR\CLSID\{32087F6B-AABF-420F-AFEB-5B1EAF4D88F2}\InprocServer32' >/dev/null 2>&1; then
    ok "PQPlatformComponent 已注册进 wine 注册表"
else
    no "PQPlatformComponent 未注册" "bash uos/03-register-pqkit.sh \"$APP_DIR\""
fi

say
say "--- 启动测试（最多 ${WAIT_SECS}s，WINEDEBUG=+loaddll,+ole,+actctx,+seh） ---"
( cd "$APP_DIR" && WINEDEBUG="+loaddll,+ole,+actctx,+seh" timeout "$WAIT_SECS" "$WINE_CMD" ./olp.exe ) >"$RUNLOG" 2>&1
rc=$?

if [ "$rc" -eq 124 ]; then
    ok "进程存活超过 ${WAIT_SECS}s（未闪退）"
elif [ "$rc" -eq 0 ]; then
    ok "进程正常退出（rc=0，通常是窗口被关闭）"
else
    no "进程提前退出，退出码 $rc" "看 $RUNLOG 第一条 err:；0xC0000135=缺运行库，0xC0000005=运行库版本混用，0xC000007B=位数不符"
fi

if grep -qiE 'REGDB_E_CLASSNOTREG|80040154' "$RUNLOG" 2>/dev/null; then
    no "日志出现 REGDB_E_CLASSNOTREG（COM 类未注册）" "重跑 03-register-pqkit.sh；仍失败则 wine regedit 导入 PQKit-com-reg.reg"
else
    ok "日志无 REGDB_E_CLASSNOTREG"
fi

if grep -qE 'err:module:import_dll' "$RUNLOG" 2>/dev/null; then
    no "存在 DLL 导入失败" "执行 grep import_dll \"$RUNLOG\" 查看缺失项，再用 winetricks 补（msxml6/gdiplus/riched20/dotnet48）"
else
    ok "无 err:module:import_dll"
fi

missing=""
for d in RPC.dll UPApiBase.dll RobotArtPQ.dll; do
    grep -qi "$d" "$RUNLOG" 2>/dev/null || missing="$missing $d"
done
if [ -z "$missing" ]; then
    ok "PQKit COM 组件已加载（RPC.dll / UPApiBase.dll / RobotArtPQ.dll）"
else
    no "以下 PQKit 组件未被加载：$missing" "说明 PQKit 初始化没走通，先解决 COM 注册与依赖缺失"
fi

if grep -qiE '5axcore|mfc140u|dmBase|Hoops' "$RUNLOG" 2>/dev/null; then
    ok "PQKit 内核 DLL 已加载（5axcore / mfc140u / dmBase / Hoops 之一）"
else
    no "未见 PQKit 内核 DLL 加载" "CoCreateInstance 之后内核没起来，查看日志中 RPC.dll 之后的报错"
fi

if grep -qiE 'unhandled exception|c0000005|page fault' "$RUNLOG" 2>/dev/null; then
    no "日志出现崩溃记录" "先确认 msvcp140/vcruntime140 为 14.4x，再试 QT_OPENGL=software、换 wine 版本"
else
    ok "日志无崩溃记录"
fi

say
say "===== 结果：PASS=$pass  FAIL=$fail ====="
if [ "$fail" -eq 0 ]; then
    say "全部通过：本机当前配置下可运行。"
    say "建议接着执行 05-pack-portable.sh，把该环境冻结成自包含包，"
    say "使其它 UOS x86_64 机器得到同样结果。"
else
    say "存在未通过项：按上面每条「下一步」处理后重跑本脚本。"
fi
say "报告：$REPORT"
say "日志：$RUNLOG"
