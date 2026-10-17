"""Upload the game data into an installed package's LocalState\\data on an Xbox.

TheSuperHackers @build githubawn 16/10/2026 One-time upload through Windows Device
Portal, so later code-only package updates (which keep LocalState) stay small.
SDL3Main (GGC_PLATFORM_UWP) prefers LocalState\\data when it holds INIZH.big.

usage: python xbox-upload-data.py <xbox-ip> <package-full-name> <zero-hour-dir> <generals-dir> [--only a.big,b.big]
"""
import http.cookiejar
import os
import ssl
import sys
import urllib.parse
import urllib.request
import uuid

ip, pkg, zh_dir, gen_dir = sys.argv[1:5]
only = None
if len(sys.argv) > 6 and sys.argv[5] == '--only':
    only = {n.lower() for n in sys.argv[6].split(',')}
base = f'https://{ip}:11443'
ctx = ssl.create_default_context()
ctx.check_hostname = False
ctx.verify_mode = ssl.CERT_NONE
jar = http.cookiejar.CookieJar()
opener = urllib.request.build_opener(urllib.request.HTTPSHandler(context=ctx),
                                     urllib.request.HTTPCookieProcessor(jar))
opener.open(f'{base}/api/os/info').read()
csrf = next((c.value for c in jar if c.name == 'CSRF-Token'), '')


def q(**kw):
    return urllib.parse.urlencode(dict(knownfolderid='LocalAppData', packagefullname=pkg, **kw))


def mkdir(parent, name):
    req = urllib.request.Request(f'{base}/api/filesystem/apps/folder?{q(path=parent, newfoldername=name)}',
                                 data=b'', method='POST', headers={'X-CSRF-Token': csrf})
    try:
        opener.open(req).read()
    except urllib.error.HTTPError as e:
        if e.code not in (400, 409):  # already exists
            raise


def upload(local, remote_dir):
    name = os.path.basename(local)
    if only is not None and name.lower() not in only:
        return
    boundary = uuid.uuid4().hex
    size = os.path.getsize(local)
    head = (f'--{boundary}\r\nContent-Disposition: form-data; name="file"; filename="{name}"\r\n'
            'Content-Type: application/octet-stream\r\n\r\n').encode()
    tail = f'\r\n--{boundary}--\r\n'.encode()

    def body():
        yield head
        with open(local, 'rb') as f:
            while True:
                chunk = f.read(1 << 20)
                if not chunk:
                    break
                yield chunk
        yield tail

    req = urllib.request.Request(f'{base}/api/filesystem/apps/file?{q(path=remote_dir)}',
                                 data=body(), method='POST',
                                 headers={'X-CSRF-Token': csrf,
                                          'Content-Type': f'multipart/form-data; boundary={boundary}',
                                          'Content-Length': str(len(head) + size + len(tail))})
    opener.open(req).read()
    print(f'{remote_dir}\\{name} ({size >> 20} MB)', flush=True)


mkdir('\\LocalState', 'data')
mkdir('\\LocalState\\data', 'generals')
for f in sorted(os.listdir(zh_dir)):
    if f.lower().endswith('.big'):
        upload(os.path.join(zh_dir, f), '\\LocalState\\data')
for f in sorted(os.listdir(gen_dir)):
    if f.lower().endswith('.big'):
        upload(os.path.join(gen_dir, f), '\\LocalState\\data\\generals')
# Loose Data files (cursors, scripts, water planes); movies are skipped.
mkdir('\\LocalState\\data', 'Data')
for sub in ('Cursors', 'Scripts', 'WaterPlane', 'INI'):
    src = os.path.join(zh_dir, 'Data', sub)
    if not os.path.isdir(src):
        continue
    for root, dirs, files in os.walk(src):
        rel = os.path.relpath(root, os.path.join(zh_dir, 'Data'))
        remote = '\\LocalState\\data\\Data\\' + rel.replace('/', '\\')
        parent, leaf = remote.rsplit('\\', 1)
        mkdir(parent, leaf)
        for f in files:
            upload(os.path.join(root, f), remote)
print('done')
