gnome = import('gnome')

headers = [
  'gdl.h',
  'gdl-dock.h',
  'gdl-dock-object.h',
  'gdl-dock-master.h',
  'gdl-dock-item.h',
  'gdl-dock-item-grip.h',
  'gdl-dock-layout.h',
  'gdl-dock-bar.h',
  'gdl-dock-placeholder.h',
]


enums = gnome.mkenums_simple('libgdltypebuiltins',
  sources: headers,
  header_prefix: '''#include <gdl/gdl.h>''',
  install_header: true,
  install_dir: gdl_includedir,
)

marshals = gnome.genmarshal('libgdlmarshal',
  sources: 'libgdlmarshal.list',
  prefix: 'gdl_marshal',
)

install_headers(headers, subdir: gdl_header_subdir)

sources = [
  enums,
  marshals,
  'gdl-dock-object.c',
  'gdl-dock-master.c',
  'gdl-dock.c',
  'gdl-dock-item.c',
  'gdl-dock-item-button-image.h',
  'gdl-dock-item-button-image.c',
  'gdl-dock-item-grip.h',
  'gdl-dock-item-grip.c',
  'gdl-dock-notebook.h',
  'gdl-dock-notebook.c',
  'gdl-dock-paned.c',
  'gdl-dock-paned.h',
  'deprecated/gdl-dock-tablabel.c',
  'deprecated/gdl-dock-tablabel.h',
  'gdl-dock-layout.c',
  'gdl-dock-placeholder.c',
  'gdl-dock-bar.c',
  'gdl-switcher.c',
  'gdl-switcher.h',
  'gdl-preview-window.h',
  'gdl-preview-window.c',
  'gdl-deprecated.c',
]

library('gdl-3',
  sources: sources,
  version: '5.0.9',
  include_directories: include_directories('..', '.'),
  c_args: [
    '-DGETTEXT_PACKAGE="gdl-3"',
    '-DG_LOG_DOMAIN="Gdl"',
  ],
  dependencies: [
    xml_dep,
    gtk_dep,
  ],
  install: true,
)