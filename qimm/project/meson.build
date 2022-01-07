srcs_project = [
	'config.c',
	'project.c',
]
deps_project = [
	dep_libshared_qimm,
]
lib_project = static_library(
	'project',
	srcs_project,
	dependencies: deps_project,
	include_directories: common_inc_qimm,
	pic: true,
	install: false
)
dep_libproject = declare_dependency(
	link_with: lib_project,
	include_directories: include_directories('.'),
	dependencies: deps_project
)

# --resource files for project
project_files = files(
	'types/common.yaml',
	'types/dashboard.yaml',
	'types/assistant.yaml',
)
install_data(project_files, install_dir: dir_project_qimm)

project_common = meson.current_source_dir() + '/types/common.yaml'
project_dashboard = meson.current_source_dir() + '/types/dashboard.yaml'
project_assistant = meson.current_source_dir() + '/types/assistant.yaml'
env_modmap += 'common.yaml=@0@;'.format(project_common)
env_modmap += 'dashboard.yaml=@0@;'.format(project_dashboard)
env_modmap += 'assistant.yaml=@0@;'.format(project_assistant)
