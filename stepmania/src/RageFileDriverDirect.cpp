#include "global.h"
#include "RageFileDriverDirect.h"
#include "RageUtil.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(WIN32)
#include <io.h>
#endif

#if !defined(O_BINARY)
#define O_BINARY 0
#endif

/* XXX: Drop FileDB and cache/resolve in here. */
#include "RageUtil_FileDB.h"

/* This driver handles direct file access. */

class RageFileObjDirect: public RageFileObj
{
private:
	int fd;

public:
	RageFileObjDirect( int fd, RageFile &p );
	virtual ~RageFileObjDirect();
	virtual int Read(void *buffer, size_t bytes);
	virtual int Write(const void *buffer, size_t bytes);
	virtual void Rewind();
	virtual int Seek( int offset );
	virtual int SeekCur( int offset );
	virtual int GetFileSize();
};


RageFileDriverDirect::RageFileDriverDirect( CString root_ ):
	root(root_)
{
}

RageFileObj *RageFileDriverDirect::Open( CString sPath, RageFile::OpenMode mode, RageFile &p, int &err )
{
	ResolvePath( sPath );
	int flags = O_BINARY;
	if( mode == RageFile::READ )
		flags |= O_RDONLY;
	else
		flags |= O_WRONLY|O_CREAT|O_TRUNC;
	int fd = open( root + sPath, flags );
	if( fd == -1 )
	{
		err = errno;
		return NULL;
	}
		
	return new RageFileObjDirect( fd, p );
}

static bool DoStat(CString sPath, struct stat *st)
{
	TrimRight(sPath, "/\\");
	return stat(sPath.c_str(), st) != -1;
}

RageFileManager::FileType RageFileDriverDirect::GetFileType( CString sPath )
{
	ResolvePath( sPath );

	struct stat st;
	if( !DoStat(sPath, &st) )
		return RageFileManager::TYPE_NONE;

	if( st.st_mode & S_IFDIR )
		return RageFileManager::TYPE_DIR;

	return RageFileManager::TYPE_FILE;
}

int RageFileDriverDirect::GetFileSizeInBytes( CString sPath )
{
	struct stat st;
	if( !DoStat(sPath, &st) )
		return 0;

	return st.st_size;
}

int RageFileDriverDirect::GetFileModTime( CString sPath )
{
	struct stat st;
	if( !DoStat(sPath, &st) )
		return -1;

	return st.st_mtime;
}

RageFileObjDirect::RageFileObjDirect( int fd_, RageFile &p ):
	RageFileObj( p )
{
	fd = fd_;
	ASSERT( fd != -1 );
}

RageFileObjDirect::~RageFileObjDirect()
{
	if( fd != -1 )
		close( fd );
}

int RageFileObjDirect::Read( void *buf, size_t bytes )
{
	int ret = read( fd, buf, bytes );
	if( ret == -1 )
	{
		SetError( strerror(errno) );
		return -1;
	}

	return ret;
}

int RageFileObjDirect::Write( const void *buf, size_t bytes )
{
	int ret = write( fd, buf, bytes );
	if( ret == -1 )
	{
		SetError( strerror(errno) );
		return -1;
	}

	return ret;
}

void RageFileObjDirect::Rewind()
{
	lseek( fd, 0, SEEK_SET );
}

int RageFileObjDirect::Seek( int offset )
{
	return lseek( fd, offset, SEEK_SET );
}

int RageFileObjDirect::SeekCur( int offset )
{
	return lseek( fd, offset, SEEK_CUR );
}

int RageFileObjDirect::GetFileSize()
{
	const int OldPos = lseek( fd, 0, SEEK_CUR );
	const int ret = lseek( fd, 0, SEEK_END );
	lseek( fd, OldPos, SEEK_SET );
	return ret;
}

/*
 * Copyright (c) 2003 by the person(s) listed below.  All rights reserved.
 *   Glenn Maynard
 */

