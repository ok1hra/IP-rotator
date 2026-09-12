#!/usr/bin/env bash
set -euo pipefail

# Exercise the WIFILT spot mailbox (/setDxcSpots, /readDxcSpots) against a real
# rotator on the bench. Checks the limits that are cheap to get wrong and
# expensive to notice: a rejected POST must never destroy the blob already
# stored, the 120 cap must hold, and the 10 minute expiry must clear it.
#
# Usage: tools/dxcspots-endpoint-check.sh <host[:port]> [web-auth-key]
#
#   host        the rotator's AJAX server, e.g. 192.168.1.50:88
#   key         the web auth password, only when WebAuthEnabled is on
#
# Test 6 (expiry) is skipped unless EXPIRY_WAIT=1 is set, because it waits out
# the full DXC_SPOT_MAX_AGE_MS.

HOST="${1:-}"
KEY="${2:-}"

if [ -z "$HOST" ]; then
  echo "Usage: $0 <host[:port]> [web-auth-key]" >&2
  exit 2
fi

BASE="http://${HOST}"
QS=""
[ -n "$KEY" ] && QS="?key=${KEY}"

PASS=0
FAIL=0

ok()   { PASS=$((PASS+1)); printf '  ok   %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf '  FAIL %s\n     %s\n' "$1" "${2:-}"; }

post() {   # post <body> [querystring-override] -> prints HTTP status
  local body="$1" qs="${2-$QS}"
  curl -s -o /dev/null -w '%{http_code}' \
       -X POST -H 'Content-Type: text/plain;charset=UTF-8' \
       --data-binary "$body" "${BASE}/setDxcSpots${qs}"
}

read_blob() {
  curl -s ${KEY:+--digest -u "admin:${KEY}"} "${BASE}/readDxcSpots"
}

spots_json() {   # spots_json <count> -> {"telnet":true,"spots":[…]}
  python3 - "$1" <<'PY'
import json,sys
n=int(sys.argv[1])
print(json.dumps({"telnet":True,"spots":[
    {"dx":"OK%dABC"%(i%10),"freq":"14025.0","type":"CQ","time":"1830"} for i in range(n)]}))
PY
}

count_spots() { python3 -c 'import json,sys; print(len(json.load(sys.stdin).get("spots",[])))'; }
field()       { python3 -c "import json,sys; print(json.load(sys.stdin).get('$1'))"; }

echo "Mailbox checks against ${BASE}"

# 1 - a valid snapshot is stored and read back
code=$(post "$(spots_json 3)")
if [ "$code" = "200" ]; then ok "valid POST accepted"; else bad "valid POST accepted" "got $code"; fi
blob=$(read_blob)
n=$(printf '%s' "$blob" | count_spots)
age=$(printf '%s' "$blob" | field ageMs)
if [ "$n" = "3" ]; then ok "GET returns the 3 spots"; else bad "GET returns the 3 spots" "got $n"; fi
if [ "$age" -ge 0 ] && [ "$age" -lt 15000 ]; then ok "ageMs is fresh ($age ms)"; else bad "ageMs is fresh" "got $age"; fi
if [ "$(printf '%s' "$blob" | field telnet)" = "True" ]; then ok "telnet flag carried through"; else bad "telnet flag carried through"; fi

# 2 - oversized body is refused and the good blob survives
code=$(post "$(python3 -c 'print("{\"telnet\":true,\"spots\":[" + ",".join(["{\"dx\":\"OK1ABC\",\"freq\":\"14025.0\",\"type\":\"CQ\",\"time\":\"1830\"}"]*200) + "]}")')")
if [ "$code" = "413" ]; then ok "9 kB body rejected with 413"; else bad "9 kB body rejected with 413" "got $code"; fi
if [ "$(read_blob | count_spots)" = "3" ]; then ok "413 left the previous blob intact"; else bad "413 left the previous blob intact"; fi

# 3 - truncated JSON is refused and the good blob survives
code=$(post '{"telnet":true,"spots":[{"dx":"OK1ABC","freq":"14025.0"')
if [ "$code" = "400" ]; then ok "truncated JSON rejected with 400"; else bad "truncated JSON rejected with 400" "got $code"; fi
if [ "$(read_blob | count_spots)" = "3" ]; then ok "400 left the previous blob intact"; else bad "400 left the previous blob intact"; fi

# 4 - over the cap the snapshot is trimmed, not refused
code=$(post "$(spots_json 200)" )
if [ "$code" = "413" ]; then
  ok "200 spots exceed the byte cap first (413) - expected, cap tested below"
else
  if [ "$code" = "200" ]; then ok "150-spot POST accepted"; else bad "150-spot POST accepted" "got $code"; fi
fi
code=$(post "$(spots_json 150)")
if [ "$code" = "200" ]; then
  n=$(read_blob | count_spots)
  if [ "$n" = "120" ]; then ok "150 spots trimmed to 120"; else bad "150 spots trimmed to 120" "got $n"; fi
else
  bad "150-spot POST accepted" "got $code"
fi

# 5 - the key is enforced only when web auth is on
if [ -n "$KEY" ]; then
  code=$(post "$(spots_json 1)" "")
  if [ "$code" = "403" ]; then ok "missing key rejected with 403"; else bad "missing key rejected with 403" "got $code"; fi
  code=$(post "$(spots_json 1)" "?key=wrong-${KEY}")
  if [ "$code" = "403" ]; then ok "wrong key rejected with 403"; else bad "wrong key rejected with 403" "got $code"; fi
  code=$(post "$(spots_json 2)")
  if [ "$code" = "200" ]; then ok "correct key accepted"; else bad "correct key accepted" "got $code"; fi
else
  echo "  skip web auth checks (no key given)"
fi

# 6 - the blob expires
if [ "${EXPIRY_WAIT:-0}" = "1" ]; then
  echo "  waiting out DXC_SPOT_MAX_AGE_MS (10 min)..."
  sleep 610
  blob=$(read_blob)
  if [ "$(printf '%s' "$blob" | field ageMs)" = "-1" ] && [ "$(printf '%s' "$blob" | count_spots)" = "0" ]; then
    ok "blob dropped after 10 minutes"
  else
    bad "blob dropped after 10 minutes" "$blob"
  fi
else
  echo "  skip expiry check (set EXPIRY_WAIT=1 to run it, takes 10 min)"
fi

echo
echo "passed ${PASS}, failed ${FAIL}"
[ "$FAIL" -eq 0 ]
