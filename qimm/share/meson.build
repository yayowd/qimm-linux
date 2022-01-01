srcs_libshared_qimm = [
	'file.c',
	'share.c',
	'yaml.c',
]
deps_libshared_qimm = [
	dep_pixman,
	dep_yaml,
	dep_libshared,
	dep_libweston_public,
	dep_libexec_weston,
	dep_lib_desktop,
]
lib_libshared_qimm = static_library(
	'shared-qimm',
	srcs_libshared_qimm,
	dependencies: deps_libshared_qimm,
	include_directories: common_inc_qimm,
	pic: true,
	install: false
)
dep_libshared_qimm = declare_dependency(
	link_with: lib_libshared_qimm,
	include_directories: include_directories('.'),
	dependencies: deps_libshared_qimm
)
