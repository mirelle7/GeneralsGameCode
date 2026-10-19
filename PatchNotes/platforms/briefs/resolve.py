"""resolve.py FILE CHOICE[,CHOICE...]  -- choice per conflict hunk: ours | theirs | base | both"""
import sys, re
path, choices = sys.argv[1], sys.argv[2].split(',')
data = open(path, 'rb').read().decode('utf-8', 'surrogateescape')
nl = '\r\n' if '\r\n' in data else '\n'
lines = data.split(nl); out = []; i = 0; k = 0
while i < len(lines):
    if lines[i].startswith('<<<<<<< '):
        o, b, t, cur = [], [], [], None; cur = o; i += 1
        while not lines[i].startswith('>>>>>>> '):
            if lines[i].startswith('||||||| '): cur = b
            elif lines[i] == '=======': cur = t
            else: cur.append(lines[i])
            i += 1
        c = choices[k] if k < len(choices) else choices[-1]; k += 1
        out += {'ours': o, 'theirs': t, 'base': b, 'both': o + t}[c]
    else: out.append(lines[i])
    i += 1
open(path, 'wb').write(nl.join(out).encode('utf-8', 'surrogateescape'))
print(path, 'resolved', k, 'hunks')
