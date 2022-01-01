srcs_shell = [
	'client.c',
	'desktop.c',
	'layer.c',
	'layout.c',
	'output.c',
	'process.c',
	'shell.c',
	qimm_desktop_shell_server_protocol_h,
	qimm_desktop_shell_protocol_c,
]
deps_shell = [
	dep_libproject,
	dep_libshared_qimm,
]
lib_shell = shared_library(
	'qimm-shell',
	srcs_shell,
	dependencies: deps_shell,
	include_directories: common_inc_qimm,
	name_prefix: '',
	install_rpath: '$ORIGIN',
	install_dir: dir_module_weston,
	install: true,
)
env_modmap += 'qimm-shell.so=@0@;'.format(lib_shell.full_path())
