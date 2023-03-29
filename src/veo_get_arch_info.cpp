/**
 * Copyright (C) 2021 NEC Corporation
 * This file is part of the AVEO.
 *
 * The AVEO is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * The AVEO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the AVEO; if not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * @file veo_get_arch_info.cpp
 */

#include "ProcHandle.hpp"

#include "veo_get_arch_info.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

static int
get_num_from_vestr(const char *str)
{
  char *endp;
  int num;

  if (strncmp(str, "ve", 2) != 0)
    return -1;
  num = (int)strtol(str+2, &endp, 10);
  if (*endp != '\0' && *endp != '\n')
    return -1;

  return num;
}

/**
 * @brief read ARCH_FILE and return architecture number.
 * @return > 0 upon success; -1 upon failure.
 */
int
veo_arch_number_sysfs(int ve_node_number)
{
  int fd;
  char arch_path[ARCH_PATH_BSIZE];
  char buf[ARCH_FILE_BSIZE];
  DIR *dp;
  struct dirent *dent;
  int ve = -1;
  int num = -1;

  dp = opendir(CLASS_VE);

  if (dp == nullptr) {
    VEO_DEBUG("ve_arch_number_sysfs:opendir error:%s", CLASS_VE);
    goto err;
  }

  while ((dent = readdir(dp)) != NULL) {
    ve = get_num_from_vestr(dent->d_name);
    if (ve_node_number != -1) {
      if (ve == ve_node_number)
	break;
    } else {
      if (ve != -1)
        break;
    }
    ve = -1;
  }
  closedir(dp);
  if (ve == -1) {
    VEO_DEBUG("ve_arch_number_sysfs:%s/veN dir is not found", CLASS_VE);
    goto err;
  }

  snprintf(arch_path,ARCH_PATH_BSIZE, CLASS_VE "/ve%d/" ARCH_FILE, ve);

  VEO_DEBUG("ve_arch_number_sysfs:path=%s", arch_path);

  fd = open(arch_path, O_RDONLY);
  if (fd == -1) {
    if (errno == ENOENT) {
      num = 1;
    }
    goto err;
  }
  if (read(fd, buf, ARCH_FILE_BSIZE) == -1) {
    VEO_DEBUG("ve_arch_number_sysfs:read error(%d) %s", errno, strerror(errno));
    close(fd);
    goto err;
  }
  close(fd);

  /*
   * "ve[0-9]+\n"
  */
  num = get_num_from_vestr(buf);

err:
  return num;
}
