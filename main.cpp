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

#include <boost/regex.hpp>

/*
#include <list>
#include <stdexcept>
#include <sstream>
#include <memory>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
*/

#include <stdio.h>

const std::string directory_boot = "/boot";
const std::string directory_modules = "/lib/modules";
const std::string directory_src = "/usr/src";

const std::string prefix_boot_config = "config-";
const std::string prefix_boot_map = "System.map-";
const std::string prefix_boot_image = "vmlinuz-";

const std::string prefix_src = "linux-";

const std::string regex_files_boot_check = "^(?:(?:" + prefix_boot_config + ")|(?:" + prefix_boot_map + ")|(?:" + prefix_boot_image + ")).+$";

struct version_info
{
	version_info(const std::vector<unsigned int> &l_version, const std::string &l_postfix = std::string())
		: version(l_version),
		postfix(l_postfix)
	{
	}

	std::vector<unsigned int> version;
	std::string postfix;
};

bool operator<(const version_info &a, const version_info &b)
{
	std::vector<unsigned int>::const_iterator iter_a, iter_b, iter_a_end, iter_b_end;

	iter_a_end = a.version.end();
	iter_b_end = b.version.end();

	for (iter_a = a.version.begin(), iter_b = b.version.begin(); (iter_a != iter_a_end) && (iter_b != iter_b_end); ++iter_a, ++iter_b)
	{
		if (*iter_a != *iter_b)
		{
			return (*iter_a < *iter_b);
		}
	}

	if (iter_a != iter_a_end)
	{
		return true;
	}
	else if (iter_b != iter_b_end)
	{
		return false;
	}
	else
	{
		return (a.postfix < b.postfix);
	}
}

std::set<std::string> list_files_in_directory(const std::string &location, const std::string &filter_regex)
{
	std::set<std::string> files;
	struct stat buffer;

	const boost::regex reg_expr(filter_regex);

	if (stat(location.c_str(), &buffer) != -1)
	{
		if (S_ISDIR(buffer.st_mode))
		{
			DIR *dirp;
			struct dirent *dp;

			if ((dirp = opendir(location.c_str())) == NULL)
			{
				return files;
			}

			try
			{
				do
				{
					if ((dp = readdir(dirp)) != NULL)
					{
						if ((strcmp(dp->d_name,".") != 0) && (strcmp(dp->d_name,"..") != 0) && (filter_regex.empty() || boost::regex_match(dp->d_name, reg_expr)))
						{
							files.insert(dp->d_name);
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

	return files;
}

int main(int argc, char **argv)
{
	std::set<std::string> files = list_files_in_directory("/boot", regex_files_boot_check);

	printf("Files in boot:\n");

	for (auto iter = files.begin(); iter != files.end(); ++iter)
	{
		printf("%s\n", iter->c_str());
	}

/*
	try
	{

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
*/

	return 0;
}
