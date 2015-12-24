/*
 * Copyright (C) 2016 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *
 * This file is part of DT Kernel Cleaner.
 *
 * DT Kernel Cleaner is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DT Kernel Cleaner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DT Kernel Cleaner.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>
#include <set>
#include <functional>

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/utsname.h>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/optional.hpp>

const std::string directory_boot = "/boot";
const std::string directory_modules = "/lib/modules";
const std::string directory_src = "/usr/src";

const std::string prefix_boot_config = "config-";
const std::string prefix_boot_map = "System.map-";
const std::string prefix_boot_image = "vmlinuz-";

const std::string prefix_src = "linux-";

const std::string regex_version = "\\d+(?:\\.\\d+)*";
const std::string regex_revision_and_local_version = "(?:-|_)\\S+";

const std::string regex_files_boot_check = "^(?:(?:" + prefix_boot_config + ")|(?:" + prefix_boot_map + ")|(?:" + prefix_boot_image + "))" + regex_version + regex_revision_and_local_version + "$";
const std::string regex_files_modules_check = "^" + regex_version + regex_revision_and_local_version + "$";
const std::string regex_files_src_check = "^(?:" + prefix_src + ")" + regex_version + regex_revision_and_local_version + "$";

const std::string regex_files_boot_capture = "^(?:(?:" + prefix_boot_config + ")|(?:" + prefix_boot_map + ")|(?:" + prefix_boot_image + "))(" + regex_version + ")(" + regex_revision_and_local_version + ")$";
const std::string regex_files_modules_capture = "^(" + regex_version + ")(" + regex_revision_and_local_version + ")$";
const std::string regex_files_src_capture = "^(?:" + prefix_src + ")(" + regex_version + ")(" + regex_revision_and_local_version + ")$";
const std::string regex_input_capture = "^(" + regex_version + ")((?:" + regex_revision_and_local_version + ")?)$";

typedef unsigned int version_info_type;

std::string versionToString(const std::vector<version_info_type> &version)
{
	std::stringstream str;

	std::vector<version_info_type>::const_iterator iter, iter_end;

	iter     = version.begin();
	iter_end = version.end();

	if (iter != iter_end)
	{
		str << *iter;
		++iter;

		for (; iter != iter_end; ++iter)
		{
			str << "." << *iter;
		}
	}

	return str.str();
}

struct version_info
{
	version_info(const std::vector<version_info_type> &l_version, const std::string &l_revision, const std::string &l_local_version = std::string())
		: version(l_version),
		revision(l_revision),
		local_version(l_local_version)
	{
	}

	std::vector<version_info_type> version;
	std::string revision;
	std::string local_version;

	std::string toString() const
	{
		return versionToString(version) + revision + local_version;
	}
};

struct VersionLess: std::binary_function<std::vector<version_info_type>, std::vector<version_info_type>, bool>
{
	bool operator()(const std::vector<version_info_type> &lhs, const std::vector<version_info_type> &rhs) const
	{
		std::vector<version_info_type>::const_iterator iter_lhs, iter_rhs, iter_lhs_end, iter_rhs_end;

		iter_lhs_end = lhs.end();
		iter_rhs_end = rhs.end();

		for (iter_lhs = lhs.begin(), iter_rhs = rhs.begin(); (iter_lhs != iter_lhs_end) && (iter_rhs != iter_rhs_end); ++iter_lhs, ++iter_rhs)
		{
			if (*iter_lhs != *iter_rhs)
			{
				return (*iter_lhs < *iter_rhs);
			}
		}

		return (iter_rhs != iter_rhs_end);
	}
};

bool operator<(const version_info &a, const version_info &b)
{
	std::vector<version_info_type>::const_iterator iter_a, iter_b, iter_a_end, iter_b_end;

	iter_a_end = a.version.end();
	iter_b_end = b.version.end();

	for (iter_a = a.version.begin(), iter_b = b.version.begin(); (iter_a != iter_a_end) && (iter_b != iter_b_end); ++iter_a, ++iter_b)
	{
		if (*iter_a != *iter_b)
		{
			return (*iter_a < *iter_b);
		}
	}

	if (iter_b != iter_b_end)
	{
		return true;
	}
	else if (iter_a != iter_a_end)
	{
		return false;
	}
	else if (a.revision < b.revision)
	{
		return true;
	}
	else if (b.revision < a.revision)
	{
		return false;
	}
	else
	{
		return (a.local_version < b.local_version);
	}
}

std::set<std::string> list_files_in_directory(const std::string &location, const std::string &filter_regex = std::string())
{
	std::set<std::string> files;
	struct stat buffer;

	const boost::regex reg_expr(filter_regex);

	if (stat(location.c_str(), &buffer) != -1)
	{
		if (S_ISDIR(buffer.st_mode))
		{
			DIR *dirp;

			if ((dirp = opendir(location.c_str())) != NULL)
			{
				try
				{
					struct dirent *dp;

					for (;;)
					{
						dp = readdir(dirp);

						if (dp == NULL)
						{
							break;
						}

						if ((strcmp(dp->d_name,".") != 0) && (strcmp(dp->d_name,"..") != 0) && (filter_regex.empty() || boost::regex_match(dp->d_name, reg_expr)))
						{
							files.insert(dp->d_name);
						}
					}
				}
				catch (...)
				{
					closedir(dirp);
					throw;
				}

				closedir(dirp);
			}
		}
	}

	return files;
}

void find_all_files_and_dirs(const std::string &location, std::set<std::string> &files, std::set<std::string> &directories)
{
	struct stat buffer;

	if (lstat(location.c_str(), &buffer) != -1)
	{
		if (S_ISDIR(buffer.st_mode))
		{
			DIR *dirp;

			directories.insert(location);

			if ((dirp = opendir(location.c_str())) != NULL)
			{
				try
				{
					struct dirent *dp;

					do
					{
						if ((dp = readdir(dirp)) != NULL)
						{
							if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0))
							{
								find_all_files_and_dirs(location + "/" + std::string(dp->d_name), files, directories);
							}
						}
					} while (dp != NULL);
				}
				catch (...)
				{
					closedir(dirp);
					throw;
				}

				closedir(dirp);
			}
		}
		else /* if (S_ISREG(buffer.st_mode) || S_ISLNK(buffer.st_mode)) */
		{
			files.insert(location);
		}
	}
}

std::vector<version_info_type> convertStringToVersion(const std::string &version_string)
{
	std::vector<version_info_type> version_vector;

	size_t pos = 0;
	size_t last_pos = 0;

	for (;;)
	{
		pos = version_string.find('.', last_pos);

		version_vector.push_back(boost::lexical_cast<version_info_type>(version_string.substr(last_pos, pos - last_pos)));

		if (pos == std::string::npos)
		{
			break;
		}

		last_pos = pos + 1;
	}

	return version_vector;
}

void remove_file(const std::string &file)
{
	if (unlink(file.c_str()) < 0)
	{
		fprintf(stderr, "Failed to remove file: %s\n", file.c_str());
	}
}

void remove_directory(const std::string &directory)
{
	if (rmdir(directory.c_str()) < 0)
	{
		fprintf(stderr, "Failed to remove directory: %s\n", directory.c_str());
	}
}

//       kernel version,                          kernel revision
std::map<std::vector<version_info_type>, std::set<std::string>, VersionLess> kernel_src_versions;
//       kernel version,                          kernel revision,      kernel local version
std::map<std::vector<version_info_type>, std::map<std::string, std::set<std::string> >, VersionLess> kernel_versions_tree;

void print_help(const char *name)
{
	fprintf(stderr,
	       "USAGE: %s [options] kernel_version"
	       "Options:\n"
	       "\t[-h] --help - shows this info\n"
	       "\t[-l] --list-only - list found kernel versions and exit. Do not specify kernel versions with this option\n"
	       "\t[-v] --verbose - list found files and also print actions before executing them\n"
	       "\t[-n] --dryrun - do not execute actions, only print them\n"
	       "\t[-k] --keep-vmlinuzold - do not remove vmlinuz.old symlink if it becomes obsolete\n"
	       "\t[-c] --clean-old - remove all kernels except the one currently running\n"
	       "\t[-s] --keep-sources - keep sources even if no kernel is built out of those sources is present\n"
	       "\n"
	       "\tkernel version is in format d.d.d-revision or just d.d.d (number of digits is variable)\n",
	       name);
}

int main(int argc, char **argv)
{
	try
	{
		bool list_only = false;
		bool verbose = false;
		bool dry_run = false;
		bool help = false;
		bool do_not_touch_vmlinuzold = false;
		bool clean_old = false;
		bool keep_sources = false;

		std::set<version_info> selected_kernels;

		for (int i = 1; i < argc; ++i)
		{
			if ((strcmp(argv[i],"--help") == 0) || (strcmp(argv[i], "-h") == 0))
			{
				help = true;
			}
			else if ((strcmp(argv[i],"--list-only") == 0) || (strcmp(argv[i], "-l") == 0))
			{
				list_only = true;
			}
			else if ((strcmp(argv[i],"--verbose") == 0) || (strcmp(argv[i], "-v") == 0))
			{
				verbose = true;
			}
			else if ((strcmp(argv[i],"--dryrun") == 0) || (strcmp(argv[i], "-n") == 0))
			{
				dry_run = true;
			}
			else if ((strcmp(argv[i],"--keep-vmlinuzold") == 0) || (strcmp(argv[i], "-k") == 0))
			{
				do_not_touch_vmlinuzold = true;
			}
			else if ((strcmp(argv[i],"--clean-old") == 0) || (strcmp(argv[i], "-c") == 0))
			{
				clean_old = true;
			}
			else if ((strcmp(argv[i],"--keep-sources") == 0) || (strcmp(argv[i], "-s") == 0))
			{
				keep_sources = true;
			}
			else
			{
				boost::cmatch reg_results;

				if (boost::regex_match(argv[i], reg_results, boost::regex(regex_input_capture)))
				{
					version_info version(convertStringToVersion(reg_results.str(1)), reg_results.str(2));

					if (selected_kernels.find(version) == selected_kernels.end())
					{
						selected_kernels.insert(version);
					}
					else
					{
						fprintf(stderr, "Kernel version is specified multiple times: %s\n", version.toString().c_str());
						return 0;
					}
				}
				else
				{
					fprintf(stderr, "Unknown option or invalid format of kernel version: %s, try %s --help for more information\n", argv[i], argv[0]);
					return 0;
				}
			}
		}

		if (help)
		{
			print_help(argv[0]);
			return 0;
		}

		if ((!list_only) && selected_kernels.empty() && (!clean_old))
		{
			fprintf(stderr, "Error: no kernel versions or other actions are specified. Try %s --help for more information\n", argv[0]);
			return -1;
		}

		if (list_only && (!selected_kernels.empty()) && clean_old)
		{
			fprintf(stderr, "Error: too much incompatible action options are specified. Try %s --help for more information\n", argv[0]);
			return -1;
		}

		// First, get all kernel source versions from /usr/src. There's no way to differ between revision and local version without checking against available kernel source versions
		if (verbose)
		{
			printf("Directories in %s:\n", directory_src.c_str());
		}

		std::set<std::string> files = list_files_in_directory(directory_src, regex_files_src_check);

		for (auto iter = files.begin(); iter != files.end(); ++iter)
		{
			if (verbose)
			{
				printf("%s\n", iter->c_str());
			}

			boost::smatch reg_results;

			if (boost::regex_match(*iter, reg_results, boost::regex(regex_files_src_capture)))
			{
				std::vector<version_info_type> version_vector = convertStringToVersion(reg_results.str(1));
				std::string revision_string = reg_results.str(2);

				kernel_src_versions[version_vector].insert(revision_string);
			}
		}

		// Now check /boot
		if (verbose)
		{
			printf("\nFiles in %s:\n", directory_boot.c_str());
		}

		files = list_files_in_directory(directory_boot, regex_files_boot_check);

		for (auto iter = files.begin(); iter != files.end(); ++iter)
		{
			if (verbose)
			{
				printf("%s\n", iter->c_str());
			}

			boost::smatch reg_results;

			if (boost::regex_match(*iter, reg_results, boost::regex(regex_files_boot_capture)))
			{
				std::vector<version_info_type> version_vector = convertStringToVersion(reg_results.str(1));
				std::string revision_and_local_version_string = reg_results.str(2);

				auto kernel_src_version = kernel_src_versions.find(version_vector);
				if (kernel_src_version != kernel_src_versions.end())
				{
					auto kernel_src_revision = kernel_src_version->second.begin();
					auto kernel_src_revision_end = kernel_src_version->second.end();

					for ( ; kernel_src_revision != kernel_src_revision_end; ++kernel_src_revision)
					{
						if (kernel_src_revision->compare(0, std::string::npos, revision_and_local_version_string, 0, kernel_src_revision->length()) == 0)
						{
							break;
						}
					}

					if (kernel_src_revision != kernel_src_revision_end)
					{
						kernel_versions_tree[version_vector][*kernel_src_revision].insert(revision_and_local_version_string.substr(kernel_src_revision->length()));
					}
					else
					{
						kernel_versions_tree[version_vector][revision_and_local_version_string].insert(std::string());
					}
				}
				else
				{
					kernel_versions_tree[version_vector][revision_and_local_version_string].insert(std::string());
				}
			}
		}

		// Now check /lib/modules
		if (verbose)
		{
			printf("\nDirectories in %s:\n", directory_modules.c_str());
		}

		files = list_files_in_directory(directory_modules, regex_files_modules_check);

		for (auto iter = files.begin(); iter != files.end(); ++iter)
		{
			if (verbose)
			{
				printf("%s\n", iter->c_str());
			}

			boost::smatch reg_results;

			if (boost::regex_match(*iter, reg_results, boost::regex(regex_files_modules_capture)))
			{
				std::vector<version_info_type> version_vector = convertStringToVersion(reg_results.str(1));
				std::string revision_and_local_version_string = reg_results.str(2);

				auto kernel_src_version = kernel_src_versions.find(version_vector);
				if (kernel_src_version != kernel_src_versions.end())
				{
					auto kernel_src_revision = kernel_src_version->second.begin();
					auto kernel_src_revision_end = kernel_src_version->second.end();

					for ( ; kernel_src_revision != kernel_src_revision_end; ++kernel_src_revision)
					{
						if (kernel_src_revision->compare(0, std::string::npos, revision_and_local_version_string, 0, kernel_src_revision->length()) == 0)
						{
							break;
						}
					}

					if (kernel_src_revision != kernel_src_revision_end)
					{
						kernel_versions_tree[version_vector][*kernel_src_revision].insert(revision_and_local_version_string.substr(kernel_src_revision->length()));
					}
					else
					{
						kernel_versions_tree[version_vector][revision_and_local_version_string].insert(std::string());
					}
				}
				else
				{
					kernel_versions_tree[version_vector][revision_and_local_version_string].insert(std::string());
				}
			}
		}

		if (verbose)
		{
			printf("\n");
		}

		if (list_only)
		{
			printf("kernel source tree for versions:\n");

			for (auto iter_version = kernel_src_versions.begin(); iter_version != kernel_src_versions.end(); ++iter_version)
			{
				for (auto iter_revision = iter_version->second.begin(); iter_revision != iter_version->second.end(); ++iter_revision)
				{
					printf("%s\n", version_info(iter_version->first, *iter_revision).toString().c_str());
				}
			}

			printf("\nkernel image and module versions:\n");

			for (auto iter_version = kernel_versions_tree.begin(); iter_version != kernel_versions_tree.end(); ++iter_version)
			{
				for (auto iter_revision = iter_version->second.begin(); iter_revision != iter_version->second.end(); ++iter_revision)
				{
					for (auto iter_local_version = iter_revision->second.begin(); iter_local_version != iter_revision->second.end(); ++iter_local_version)
					{
						printf("%s\n", version_info(iter_version->first, iter_revision->first, *iter_local_version).toString().c_str());
					}
				}
			}
		}
		else
		{
			if (clean_old)
			{
				struct utsname name;

				if (uname(&name) == -1)
				{
					throw std::runtime_error("uname() call failed");
				}

				std::string current_version = name.release;

				boost::smatch reg_results;

				if (!boost::regex_match(current_version, reg_results, boost::regex(regex_input_capture)))
				{
					std::stringstream str;
					str << "Failed to parse version string returned by uname(): " << current_version;
					throw std::runtime_error(str.str());
				}

				version_info version(convertStringToVersion(reg_results.str(1)), reg_results.str(2));

				selected_kernels.clear();

				auto kernel_version = kernel_versions_tree.begin();
				auto kernel_version_end = kernel_versions_tree.end();

				for ( ; kernel_version != kernel_version_end; ++kernel_version)
				{
					auto kernel_revision = kernel_version->second.begin();
					auto kernel_revision_end = kernel_version->second.end();

					for ( ; kernel_revision != kernel_revision_end; ++kernel_revision)
					{
						auto kernel_local_version = kernel_revision->second.begin();
						auto kernel_local_version_end = kernel_revision->second.end();

						for ( ; kernel_local_version != kernel_local_version_end; ++kernel_local_version)
						{
							version_info found_version(kernel_version->first, kernel_revision->first, *kernel_local_version);

							if (version.toString() != found_version.toString())
							{
								selected_kernels.insert(found_version);
							}
						}
					}
				}
			}

			auto kernel_version_iter = selected_kernels.begin();
			auto kernel_version_iter_end = selected_kernels.end();

			for (; kernel_version_iter != kernel_version_iter_end; ++kernel_version_iter)
			{
				boost::optional<version_info> found_kernel;
				boost::optional<version_info> found_kernel_sources;

				{
					std::vector<version_info_type> version_vector = kernel_version_iter->version;
					std::string revision_and_local_version_string = kernel_version_iter->revision + kernel_version_iter->local_version;

					auto kernel_version = kernel_versions_tree.find(version_vector);
					if (kernel_version != kernel_versions_tree.end())
					{
						auto kernel_revision = kernel_version->second.begin();
						auto kernel_revision_end = kernel_version->second.end();

						for ( ; kernel_revision != kernel_revision_end; ++kernel_revision)
						{
							if (kernel_revision->first.compare(0, std::string::npos, revision_and_local_version_string, 0, kernel_revision->first.length()) == 0)
							{
								std::string kernel_local_version_string = revision_and_local_version_string.substr(kernel_revision->first.length());

								auto kernel_local_version = kernel_revision->second.find(kernel_local_version_string);
								if (kernel_local_version != kernel_revision->second.end())
								{
									found_kernel = version_info(version_vector, kernel_revision->first, kernel_local_version_string);
									break;
								}
							}
						}
					}

					auto kernel_src_version = kernel_src_versions.find(version_vector);
					if (kernel_src_version != kernel_src_versions.end())
					{
						auto kernel_src_revision = kernel_src_version->second.begin();
						auto kernel_src_revision_end = kernel_src_version->second.end();

						for ( ; kernel_src_revision != kernel_src_revision_end; ++kernel_src_revision)
						{
							if (*kernel_src_revision == revision_and_local_version_string)
							{
								found_kernel_sources = version_info(version_vector, revision_and_local_version_string);
								break;
							}
						}
					}
				}

				if (found_kernel)
				{
					std::string version_str = kernel_version_iter->toString();
					printf("Removing kernel version %s\n", version_str.c_str());

					std::set<std::string> files;
					std::set<std::string> directories;

					find_all_files_and_dirs(directory_modules + "/" + version_str, files, directories);

					// clean everything in /boot
					if (verbose)
					{
						printf("Removing file %s\n", (directory_boot + "/" + prefix_boot_config + version_str).c_str());
					}

					if (!dry_run)
					{
						remove_file(directory_boot + "/" + prefix_boot_config + version_str);
					}

					if (verbose)
					{
						printf("Removing file %s\n", (directory_boot + "/" + prefix_boot_map + version_str).c_str());
					}

					if (!dry_run)
					{
						remove_file(directory_boot + "/" + prefix_boot_map + version_str);
					}

					if (verbose)
					{
						printf("Removing file %s\n", (directory_boot + "/" + prefix_boot_image + version_str).c_str());
					}

					if (!dry_run)
					{
						remove_file(directory_boot + "/" + prefix_boot_image + version_str);
					}

					// clean everything in /lib/modules
					auto files_end = files.end();
					for (auto files_cur = files.begin(); files_cur != files_end; ++files_cur)
					{
						if (verbose)
						{
							printf("Removing file %s\n", files_cur->c_str());
						}

						if (!dry_run)
						{
							remove_file(*files_cur);
						}
					}

					// go in reverse order to make sure that top-most directories are removed last
					auto dirs_end = directories.rend();
					for (auto dirs_cur = directories.rbegin(); dirs_cur != dirs_end; ++dirs_cur)
					{
						if (verbose)
						{
							printf("Removing dir %s\n", dirs_cur->c_str());
						}

						if (!dry_run)
						{
							remove_directory(*dirs_cur);
						}
					}

					// remove kernel from lists
					kernel_versions_tree[found_kernel->version][found_kernel->revision].erase(found_kernel->local_version);
				}

				// TODO: when removing kernel sources make sure to remove all kernel files built from these sources
				if ((!keep_sources)
					&& ((found_kernel && kernel_versions_tree[found_kernel->version][found_kernel->revision].empty())
						|| ((!found_kernel) && found_kernel_sources)))
				{
					std::string version_str;

					if (found_kernel)
					{
						version_str = versionToString(found_kernel->version) + found_kernel->revision;
					}
					else
					{
						version_str = found_kernel_sources->toString();
					}

					printf("Removing kernel sources version %s\n", version_str.c_str());

					std::set<std::string> files;
					std::set<std::string> directories;

					find_all_files_and_dirs(directory_src + "/" + prefix_src + version_str, files, directories);

					// clean everything in /usr/src
					auto files_end = files.end();
					for (auto files_cur = files.begin(); files_cur != files_end; ++files_cur)
					{
						if (verbose)
						{
							printf("Removing file %s\n", files_cur->c_str());
						}

						if (!dry_run)
						{
							remove_file(*files_cur);
						}
					}

					// go in reverse order to make sure that top-most directories are removed last
					auto dirs_end = directories.rend();
					for (auto dirs_cur = directories.rbegin(); dirs_cur != dirs_end; ++dirs_cur)
					{
						if (verbose)
						{
							printf("Removing dir %s\n", dirs_cur->c_str());
						}

						if (!dry_run)
						{
							remove_directory(*dirs_cur);
						}
					}
				}
			}

			// TODO: this currently does not work in dry-run, i.e. it does not show that it would remove the symlink due to file it points to not being removed
			if (!do_not_touch_vmlinuzold)
			{
				const std::string vmlinuzold_name = "/boot/vmlinuz.old";
				struct stat buffer;

				if ((stat(vmlinuzold_name.c_str(), &buffer) != -1) && (S_ISLNK(buffer.st_mode)))
				{
					if (verbose)
					{
						printf("Removing file %s\n", vmlinuzold_name.c_str());
					}

					if (!dry_run)
					{
						remove_file(vmlinuzold_name);
					}
				}
			}
		}
	}
	catch (const std::exception &exc)
	{
		fprintf(stderr, "Caught std::exception: %s\n", exc.what());
		return -1;
	}
	catch (...)
	{
		fprintf(stderr, "Caught unknown exception\n");
		return -1;
	}

	return 0;
}
