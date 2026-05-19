#!/usr/bin/env bash

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

YCASHD=${YCASHD:-$SRCDIR/ycashd}
YCASHD_WALLET_TOOL=${YCASHD_WALLET_TOOL:-$SRCDIR/ycashd-wallet-tool}
YCASHCLI=${YCASHCLI:-$SRCDIR/ycash-cli}
YCASHTX=${YCASHTX:-$SRCDIR/ycash-tx}

[ ! -x $YCASHD ] && echo "$YCASHD not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
read -r -a YECVERSTR <<< "$($YCASHCLI --version | head -n1 | awk '{ print $NF }')"
read -r -a YECVER <<< "$(echo ${YECVERSTR[0]} | awk -F- '{ OFS="-"; NF--; print $0; }')"
read -r -a YECCOMMIT <<< "$(echo ${YECVERSTR[0]} | awk -F- '{ print $NF }')"

# Create a footer file with copyright content.
# This gets autodetected fine for ycashd if --version-string is not set,
# but has different outcomes for ycash-cli.
echo "[COPYRIGHT]" > footer.h2m
$YCASHD --version | sed -n '1!p' >> footer.h2m
grep -v "Bitcoin Core Developers" footer.h2m >footer-no-bitcoin-copyright.h2m

for cmd in $YCASHD $YCASHCLI $YCASHTX; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${YECVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${YECCOMMIT[0]}//g" ${MANDIR}/${cmdname}.1
done

for cmd in $YCASHD_WALLET_TOOL; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${YECVER[0]} --include=footer-no-bitcoin-copyright.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${YECCOMMIT[0]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
rm -f footer-no-bitcoin-copyright.h2m
