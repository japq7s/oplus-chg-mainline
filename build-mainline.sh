#!/bin/sh
# Build oplus_chg (v1) as an external module against a mainline kernel tree.
#   KSRC=<kernel source> KOUT=<kernel build dir> ./build-mainline.sh [make args]
# Configuration of the realme GT Neo5 150W (senna), as in the OPLUS GKI config
# for waipio, without the v2 framework: senna has no /soc/oplus_chg_core node
# with oplus,chg_framework_v2, so only v1 is used on this phone.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
KSRC=${KSRC:?KSRC=kernel source}
KOUT=${KOUT:?KOUT=kernel build directory}

CHG_CFG="CONFIG_OPLUS_CHARGER=y CONFIG_OPLUS_CHG=m \
CONFIG_OPLUS_SMART_CHARGE=y CONFIG_OPLUS_SM8450_CHARGER=y \
CONFIG_OPLUS_CHIP_SOC_NODE=y CONFIG_OPLUS_SHIP_MODE_SUPPORT=y \
CONFIG_OPLUS_SMART_CHARGER_SUPPORT=y CONFIG_OPLUS_CALL_MODE_SUPPORT=y \
CONFIG_OPLUS_CHECK_CHARGERID_VOLT=y CONFIG_OPLUS_CHG_DYNAMIC_CONFIG=y \
CONFIG_OPLUS_DA9313_CHARGER=y CONFIG_OPLUS_MP2762_CHARGER=y \
CONFIG_OPLUS_SGM41512_CHARGER=y \
CONFIG_OPLUS_ADSP_SM8450_CHARGER=y CONFIG_OPLUS_CHG_VOOCPHY=y \
CONFIG_OPLUS_CHG_AP_VOOCPHY=y CONFIG_OPLUS_CHG_VOOCPHY_CHGLIB=y \
CONFIG_OPLUS_VOOCPHY_SC8517=y CONFIG_OPLUS_AUDIO_SWITCH_GLINK=y"

# Kbuild turns command line CONFIG_X=y into obj- selection only; the sources
# also test them with #ifdef, so pass them to the preprocessor as well.
KCF=$(printf '%s' "$CHG_CFG" | tr ' ' '\n' | sed -n 's/^\(CONFIG_[A-Z0-9_]*\)=y$/-D\1/p' | tr '\n' ' ')
KCF="$KCF -DCONFIG_OPLUS_CHG_MODULE=1 -DOPLUS_FEATURE_CHG_BASIC \
-DCONFIG_OPLUS_SYSTEM_KERNEL_QCOM -include $here/compat/oplus_mainline_compat.h -I$here/include -I$here/v1"

# shellcheck disable=SC2086
exec make -C "$KSRC" O="$KOUT" ARCH=arm64 M="$here" \
	KCFLAGS="$KCF" $CHG_CFG "$@" modules
