#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002,2003  Josh Coalson
#
#  This program is part of FLAC; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

AUTOMAKE_OPTIONS = foreign

SUBDIRS = ru images

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)/html

doc_DATA =  \
	comparison.html \
	developers.html \
	documentation.html \
	download.html \
	features.html \
	format.html \
	goals.html \
	id.html \
	index.html \
	news.html

EXTRA_DIST = $(doc_DATA) api

# The install targets don't copy whole directories so we have to
# handle 'api/' specially:
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/api
	(cd api && $(INSTALL_DATA) * $(DESTDIR)$(docdir)/api)
uninstall-local:
	rm -rf $(DESTDIR)$(docdir)/api
maintainer-clean-local:
	rm -rf api
