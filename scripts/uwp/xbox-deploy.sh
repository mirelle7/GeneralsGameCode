#!/usr/bin/env bash
# TheSuperHackers @build githubawn 16/10/2026 Install (and optionally launch) a UWP
# package on an Xbox in Dev Mode through the Windows Device Portal REST API.
#
#   scripts/uwp/xbox-deploy.sh <xbox-ip> <package.msix> [--launch]
#
# Set WDP_USER / WDP_PASS if Device Portal authentication is enabled.
set -euo pipefail

ip="$1"
pkg="$2"
launch="${3:-}"
base="https://${ip}:11443"
jar="$(mktemp)"
trap 'rm -f "$jar"' EXIT
auth=()
if [[ -n "${WDP_USER:-}" ]]; then auth=(-u "${WDP_USER}:${WDP_PASS:-}"); fi

# Prime the CSRF cookie; Device Portal rejects POSTs without a matching header.
curl -sk "${auth[@]}" -c "$jar" -o /dev/null "$base/api/os/info"
csrf="$(awk '$6 == "CSRF-Token" { print $7 }' "$jar")"

name="$(basename "$pkg")"
echo "Uploading $name ($(du -h "$pkg" | cut -f1)) to $ip ..."
curl -sk "${auth[@]}" -b "$jar" -H "X-CSRF-Token: ${csrf}" \
    -F "${name}=@${pkg};type=application/octet-stream" \
    "$base/api/app/packagemanager/package?package=${name}" -w '\nHTTP %{http_code}\n'

# 204 while the install is running, 200 when done (body has the result).
for _ in $(seq 1 600); do
    code="$(curl -sk "${auth[@]}" -b "$jar" -o /tmp/wdp_state.json -w '%{http_code}' "$base/api/app/packagemanager/state")"
    if [[ "$code" != "204" ]]; then
        echo "Install state HTTP $code: $(cat /tmp/wdp_state.json)"
        break
    fi
    sleep 2
done

if [[ "$launch" == "--launch" ]]; then
    # Launch the package that was just installed: the newest GeneralsZH version listed,
    # not the first one (an older version can still be listed right after an update).
    info="$(curl -sk "${auth[@]}" -b "$jar" "$base/api/app/packagemanager/packages" |
        python -X utf8 -c "
import json,sys
d=json.load(sys.stdin)
p=[p for p in d['InstalledPackages'] if p['PackageFullName'].startswith('TheSuperHackers.GeneralsZH_')]
v=lambda p:(p['Version']['Major'],p['Version']['Minor'],p['Version']['Build'],p['Version']['Revision'])
p=max(p,key=v)
print(p['PackageFullName']+'|'+p['PackageRelativeId'])")"
    full="${info%%|*}"
    praid="${info##*|}"
    echo "Launching $praid ($full)"
    curl -sk "${auth[@]}" -b "$jar" -H "X-CSRF-Token: ${csrf}" -X POST -d ""         "$base/api/taskmanager/app?appid=$(printf %s "$praid" | base64 -w0)&package=$(printf %s "$full" | base64 -w0)"         -w '
HTTP %{http_code}
'
fi
