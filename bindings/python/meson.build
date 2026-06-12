python3_dep = dependency('python3', required: false)
if python3_dep.found()
  pymod = import('python')
  py_install = pymod.find_installation('python3')
  py_site = py_install.get_install_dir()
  install_data('gsni/__init__.py', install_dir: py_site / 'gsni')
endif
