# --version & build id
qimm_name = 'qimm'
qimm_version = '0.0.1'
qimm_build_id = '"@0@"'.format(qimm_version)
git = find_program('git', native: true, required: false)
if git.found()
	git_commit = run_command(git, 'rev-parse', '--short', 'HEAD', check: true)
	git_branch = run_command(git, 'rev-parse', '--abbrev-ref', 'HEAD', check: true)
	cur_date = run_command('date', '-u', env: ['LANG='], check: true)
	if git_commit.returncode() == 0 and git_branch.returncode() == 0
		qimm_build_id = '@0@-@1@ (@2@, branch \'@3@\')'.format(
			qimm_version,
			git_commit.stdout().strip(),
			cur_date.stdout().strip(),
			git_branch.stdout().strip(),
		)
	endif
endif
#   define version string to config.h
qimm_config_h = configuration_data()
qimm_config_h.set('_GNU_SOURCE', '1')
qimm_config_h.set('_ALL_SOURCE', '1')
qimm_config_h.set_quoted('QIMM_PACKAGE_STRING', 'qimm @0@'.format(qimm_version))
qimm_config_h.set_quoted('QIMM_PACKAGE_VERSION', qimm_version)
qimm_config_h.set_quoted('QIMM_NAME', qimm_name)
qimm_config_h.set_quoted('QIMM_VERSION', qimm_version)
qimm_config_h.set_quoted('QIMM_BUILD_ID', qimm_build_id)
qimm_config_h.set_quoted('QIMM_PACKAGE_URL', 'https://gitlab.freedesktop.org/yayowd/qimm')
qimm_config_h.set_quoted('QIMM_PACKAGE_BUGREPORT', 'https://gitlab.freedesktop.org/yayowd/qimm/issues')

# --directories
dir_module_qimm = join_paths(dir_lib, 'qimm')
dir_protocol_qimm = join_paths(dir_data, 'qimm', 'protocols')
dir_project_qimm = join_paths(dir_data, 'qimm', 'projects')
qimm_config_h.set_quoted('QIMM_MODULEDIR', dir_module_qimm)
qimm_config_h.set_quoted('QIMM_PROTOCOLDIR', dir_protocol_qimm)
qimm_config_h.set_quoted('QIMM_PROJECTDIR', dir_project_qimm)

# --dependencies
#   yaml
dep_yaml = dependency('yaml-0.1')

# --include headers
public_inc_qimm = include_directories('include')
common_inc_qimm = [ include_directories('.'), public_inc_qimm, common_inc ]

# --sub projects
subdir('protocol')
subdir('share')
subdir('client')
subdir('project')
subdir('shell')
subdir('compositor')

# --debug environments setup
if get_option('debug')
	qimm_config_h.set10('DEBUG', true)
endif

# --make config header file
configure_file(output: 'qimm-config.h', configuration: qimm_config_h)
