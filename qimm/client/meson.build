srcs_client = [
	'main.c',
	#weston_desktop_shell_client_protocol_h,
    #weston_desktop_shell_protocol_c,
]
deps_client = [
	dep_libshared_qimm,
	dep_toytoolkit,
]
exe_client = executable(
	'qimm-client',
	srcs_client,
	dependencies: deps_client,
	include_directories: common_inc_qimm,
	install_dir: dir_module_qimm,
	#install_dir: get_option('libexecdir'),
	install: true,
)
env_modmap += 'qimm-client=@0@;'.format(exe_client.full_path())

qimm_config_h.set_quoted('QIMM_SHELL_CLIENT', 'qimm-client')
