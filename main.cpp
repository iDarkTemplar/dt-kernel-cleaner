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

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

const std::string directory_boot = "/boot";
const std::string directory_modules = "/lib/modules";
const std::string directory_src = "/usr/src";

const std::string prefix_boot_config = "config-";
const std::string prefix_boot_map = "System.map-";
const std::string prefix_boot_image = "vmlinuz-";

const std::string prefix_src = "linux-";

const std::string regex_version = "\\d+(?:\\.\\d+)*";
const std::string regex_postfix_revision = "\\S+";

const std::string regex_files_boot_check = "^(?:(?:" + prefix_boot_config + ")|(?:" + prefix_boot_map + ")|(?:" + prefix_boot_image + "))" + regex_version + "-" + regex_postfix_revision + "$";
const std::string regex_files_modules_check = "^" + regex_version + "-" + regex_postfix_revision + "$";
const std::string regex_files_src_check = "^(?:" + prefix_src + ")" + regex_version + "-" + regex_postfix_revision + "$";

const std::string regex_files_src_capture = "^(?:" + prefix_src + ")(" + regex_version + ")-(" + regex_postfix_revision + ")$";

typedef unsigned int version_info_type;

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

			str << "-";
		}

		str << revision << local_version;

		return str.str();
	}
};

struct VersionLess
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
		/*
		if (iter_rhs != iter_rhs_end)
		{
			return true;
		}
		else if (iter_lhs != iter_lhs_end)
		{
			return false;
		}
		*/
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

std::set<version_info> src_version;
std::map<std::vector<version_info_type>, std::set<std::string>, VersionLess> kernel_src_versions;
//       kernel version,                          kernel revision,      kernel local version
std::map<std::vector<version_info_type>, std::map<std::string, std::set<std::string> >, VersionLess> kernel_versions_tree;

int main(int argc, char **argv)
{
	try
	{
		std::set<std::string> files = list_files_in_directory(directory_boot, regex_files_boot_check);

		printf("Files in %s:\n", directory_boot.c_str());

		for (auto iter = files.begin(); iter != files.end(); ++iter)
		{
			printf("%s\n", iter->c_str());
		}

		printf("\nDirectories in %s:\n", directory_modules.c_str());

		files = list_files_in_directory(directory_modules, regex_files_modules_check);

		for (auto iter = files.begin(); iter != files.end(); ++iter)
		{
			printf("%s\n", iter->c_str());
		}

		printf("\nDirectories in %s:\n", directory_src.c_str());

		// First, get all kernel source versions. There's no way to differ between revision and local version without checking against available kernel source versions
		files = list_files_in_directory(directory_src, regex_files_src_check);

		for (auto iter = files.begin(); iter != files.end(); ++iter)
		{
			printf("%s\n", iter->c_str());

			boost::smatch reg_results;

			if (boost::regex_match(*iter, reg_results, boost::regex(regex_files_src_capture)))
			{
				std::string version_string = reg_results.str(1);
				std::string revision_string = reg_results.str(2);

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

				src_version.insert(version_info(version_vector, revision_string));
				kernel_src_versions[version_vector].insert(revision_string);
				kernel_versions_tree[version_vector][revision_string].insert(std::string());
			}
		}

		for (auto iter = src_version.begin(); iter != src_version.end(); ++iter)
		{
			printf("kernel version: %s\n", iter->toString().c_str());
		}

		for (auto iter_version = kernel_src_versions.begin(); iter_version != kernel_src_versions.end(); ++iter_version)
		{
			for (auto iter_revision = iter_version->second.begin(); iter_revision != iter_version->second.end(); ++iter_revision)
			{
				printf("kernel version from map: %s\n", version_info(iter_version->first, *iter_revision).toString().c_str());
			}
		}

		for (auto iter_version = kernel_versions_tree.begin(); iter_version != kernel_versions_tree.end(); ++iter_version)
		{
			for (auto iter_revision = iter_version->second.begin(); iter_revision != iter_version->second.end(); ++iter_revision)
			{
				for (auto iter_local_version = iter_revision->second.begin(); iter_local_version != iter_revision->second.end(); ++iter_local_version)
				{
					printf("kernel version from tree: %s\n", version_info(iter_version->first, iter_revision->first, *iter_local_version).toString().c_str());
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
