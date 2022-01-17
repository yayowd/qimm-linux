srcs_project = [
	'config.c',
	'data.c',
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
	'projects/common.yaml',
	'projects/dashboard.yaml',
	'projects/assistant.yaml',
)
install_data(project_files, install_dir: dir_project_qimm)

project_common = meson.current_source_dir() + '/projects/common.yaml'
project_dashboard = meson.current_source_dir() + '/projects/dashboard.yaml'
project_assistant = meson.current_source_dir() + '/projects/assistant.yaml'
env_modmap += 'common.yaml=@0@;'.format(project_common)
env_modmap += 'dashboard.yaml=@0@;'.format(project_dashboard)
env_modmap += 'assistant.yaml=@0@;'.format(project_assistant)
