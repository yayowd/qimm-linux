srcs_qimm = [
	'main.c',
]
deps_qimm = [
	dep_libshared_qimm,
]
exe_qimm = executable(
	'qimm',
	srcs_qimm,
	dependencies: deps_qimm,
	include_directories: common_inc_qimm,
	build_rpath: dir_module_weston,
	install_rpath: dir_module_weston,
	install: true,
)
