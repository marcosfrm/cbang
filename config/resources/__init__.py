################################################################################
#                                                                              #
#         This file is part of the C! library.  A.K.A the cbang library.       #
#                                                                              #
#               Copyright (c) 2021-2025, Cauldron Development  Oy              #
#               Copyright (c) 2003-2021, Cauldron Development LLC              #
#                              All rights reserved.                            #
#                                                                              #
#        The C! library is free software: you can redistribute it and/or       #
#       modify it under the terms of the GNU Lesser General Public License     #
#      as published by the Free Software Foundation, either version 2.1 of     #
#              the License, or (at your option) any later version.             #
#                                                                              #
#       The C! library is distributed in the hope that it will be useful,      #
#         but WITHOUT ANY WARRANTY; without even the implied warranty of       #
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      #
#                Lesser General Public License for more details.               #
#                                                                              #
#        You should have received a copy of the GNU Lesser General Public      #
#                License along with the C! library.  If not, see               #
#                        <http://www.gnu.org/licenses/>.                       #
#                                                                              #
#       In addition, BSD licensing may be granted on a case by case basis      #
#       by written permission from at least one of the copyright holders.      #
#          You may request written permission by emailing the authors.         #
#                                                                              #
#                 For information regarding this software email:               #
#                                Joseph Coffland                               #
#                         joseph@cauldrondevelopment.com                       #
#                                                                              #
################################################################################

from __future__ import print_function

import os
import tarfile
import re
import platform
import time
import textwrap
import stat
import shutil

from SCons.Script import *

resource_version = 2


class ResourceContext:
  def __init__(self): pass


def start_file(ctx, f):
  note = ('WARNING: This file was auto generated.  Please do NOT '
          'edit directly or check in to source control.')

  f.write(
    '/' + ('*' * 75) + '\\\n   ' +
    '\n   '.join(textwrap.wrap(note)) + '\n' +
    '\\' + ('*' * 75) + '/\n'
    '\n'
    '#include <cbang/util/ResourceManager.h>\n\n'
    'using namespace cb;\n\n')

  if ctx.ns:
    for namespace in ctx.ns.split('::'):
      f.write('namespace %s {\n' % namespace)


def end_file(ctx, f):
  if ctx.ns:
    for namespace in ctx.ns.split('::'):
      f.write('} // namespace %s\n' % namespace)


def write_string(ctx, output, s, newline = 0):
  l = len(s)

  if newline or ctx.col + l > 80:
    output.write('\n')
    ctx.col = 0

  i = s.rfind('\n')
  if i != -1: ctx.col = l - (i + 1)
  else: ctx.col += l

  output.write(s)


def is_excluded(exclude, path):
  return exclude != None and exclude.search(path) != None


def write_resource(ctx, output, data_dir, path, children = None,
                   exclude = None):
  name = os.path.basename(path)
  if is_excluded(ctx.exclude, path): return

  is_dir = os.path.isdir(path)
  id = ctx.next_id
  ctx.next_id += 1
  length = 0

  if is_dir:
    typeStr = 'Directory'
    child_resources = []

    for filename in os.listdir(path):
      write_resource(ctx, output, data_dir, os.path.join(path, filename),
                     child_resources, exclude)

    write_string(ctx, output, 'const Resource *children%d[] = {' % id)

    for res in child_resources:
      write_string(ctx, output, '&resource%d,' % res)

    write_string(ctx, output, '0};\n')

  else:
    out_path = '%s/data%d.cpp' % (data_dir, id)
    print('Writing resource: %s to %s' % (path, out_path))

    typeStr = 'File'
    with open(path, 'rb') as f:
      prototype = 'extern const unsigned char data%d[]' % id

      write_string(ctx, output, '%s;\n' % prototype)

      with open(out_path, 'w') as out:
        start_file(ctx, out)

        write_string(ctx, out, prototype + ' = {')

        while True:
          count = 0
          for c in f.read(102400):
            if not isinstance(c, int): c = ord(c)
            write_string(ctx, out, '%d,' % c)
            count += 1

          if count == 0: break
          length += count

        write_string(ctx, out, '0};\n')

        end_file(ctx, out)

  if children != None: children.append(id)

  output.write('extern const %sResource resource%d("%s", ' %
               (typeStr, id, name))

  if is_dir: output.write('children%d' % id)
  else: output.write('(const char *)data%d, %d' % (id, length))

  output.write(');\n')


def get_exclude(env):
  pattern = None
  for ex in env.get('RESOURCES_EXCLUDES'):
    if pattern == None: pattern = ''
    else: pattern += '|'
    pattern += '(%s)' % ex

  return re.compile(pattern)


def resources_build(target, source, env):
  ctx = ResourceContext()
  ctx.env = env
  ctx.ns = env.get('RESOURCES_NS')
  ctx.exclude = get_exclude(env)
  ctx.next_id = 0
  ctx.col = 0

  target = str(target[0])

  data_dir = os.path.splitext(target)[0] + ".data"
  if os.path.exists(data_dir): shutil.rmtree(data_dir)
  os.mkdir(data_dir)

  # Write resources
  with open(target, 'w') as f:
    start_file(ctx, f)

    for src in source:
      write_resource(ctx, f, data_dir, str(src))

    nsParts = ctx.ns.lower().split('::')
    f.write(('void loadResources() {\n'
             '  cb::ResourceManager::instance().add("%s", &resource0);\n'
             '}\n') % '.'.join(nsParts))

    end_file(ctx, f)


def get_targets(exclude, path, data_dir, count = [0]):
  if is_excluded(exclude, path): return []

  if not os.path.exists(path):
    raise Exception('Resource "%s" does not exist' % path)

  id = count[0]
  count[0] += 1

  if os.path.isdir(path):
    targets = []

    for name in os.listdir(path):
      child = os.path.join(path, name)
      targets += get_targets(exclude, child, data_dir, count)

    return targets

  return Depends(File('data%d.cpp' % id, data_dir), path)


def modify_targets(target, source, env):
  exclude = get_exclude(env)
  name = str(target[0])
  base, ext = os.path.splitext(name)
  data_dir = '%s.v%d.data' % (base, resource_version)
  target   = ['%s.v%d%s' % (base, resource_version, ext)]
  count = [0]
  for s in source: target += get_targets(exclude, str(s), data_dir, count)
  return target, source


def generate(env):
  env.SetDefault(RESOURCES_NS = '')
  env.SetDefault(RESOURCES_EXCLUDES = [r'\.svn', r'~$'])

  bld = env.Builder(action = resources_build,
                    source_factory = SCons.Node.FS.Entry,
                    emitter = modify_targets)
  env.Append(BUILDERS = {'Resources' : bld})


def exists(): return True
