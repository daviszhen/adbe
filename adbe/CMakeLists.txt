# Copyright (c) 2006, 2014, Oracle and/or its affiliates. All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

SET(ADBE_PLUGIN_STATIC "adbe")
SET(ADBE_PLUGIN_MANDATORY  TRUE)

SET(EXAMPLE_SOURCES Transaction.cpp LogRecord.cpp RecoveryMgr.cpp ConcurrencyMgr.cpp
	ha_example.cc RecordPage.cpp
	blockid.h Buffer.h BufferList.h BufferMgr.h bytebuffer.h ConcurrencyMgr.h 
	FileMgr.h ha_example.h Layout.h LockTable.h LogIterator.h LogMgr.h LogRecord.h 
	RecordPage.h RecoveryMgr.h Transaction.h page.h Schema.h)
#IF(WITH_ADBE_STORAGE_ENGINE AND NOT WITHOUT_ADBE_STORAGE_ENGINE)
  MYSQL_ADD_PLUGIN(adbe ${EXAMPLE_SOURCES} STORAGE_ENGINE MANDATORY RECOMPILE_FOR_EMBEDDED)
#ELSEIF(NOT WITHOUT_ADBE_STORAGE_ENGINE)
#  MYSQL_ADD_PLUGIN(adbe ${EXAMPLE_SOURCES} STORAGE_ENGINE MODULE_ONLY)
#ENDIF()
