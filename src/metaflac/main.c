/* metaflac - Command-line FLAC metadata editor
 * Copyright (C) 2001,2002,2003  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*@@@
more powerful operations yet to add:
	add a seektable, using same args as flac
*/

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "operations.h"
#include "options.h"
#include <locale.h>

int main(int argc, char *argv[])
{
	CommandLineOptions options;
	int ret = 0;

	setlocale(LC_ALL, "");
	init_options(&options);

	if(parse_options(argc, argv, &options))
		ret = !do_operations(&options);

	free_options(&options);

	return ret;
}
