#include "global.h"
#include "RageFileManager.h"
#include "RageFileDriver.h"
#include <errno.h>

RageFileManager *FILEMAN = NULL;

struct LoadedDriver
{
	/* A loaded driver may have a base path, which modifies the path we
	 * pass to the driver.  For example, if the base is "Songs/", and we
	 * want to send the path "Songs/Foo/Bar" to it, then we actually
	 * only send "Foo/Bar".  The path "Themes/Foo" is out of the scope
	 * of the driver, and GetPath returns false. */
	RageFileDriver *driver;
	CString base;

	CString GetPath( CString path );
};

static vector<LoadedDriver> g_Drivers;


RageFileManager::RageFileManager()
{
#if defined(XBOX)
	RageFileManager::AddFS( "dir", ".", "" );
	/* XXX: drop BASE_PATH and do this instead */
//	RageFileManager::AddFS( "dir", "D:\\", "" );
#else
	/* Paths relative to the CWD: */
	RageFileManager::AddFS( "dir", ".", "" );

	/* Absolute paths.  This is rarely used, eg. by Alsa9Buf::GetSoundCardDebugInfo(). */
	RageFileManager::AddFS( "dir", "/", "/" );
#endif
}

RageFileManager::~RageFileManager()
{
	/* Note that drivers can use previously-loaded drivers, eg. to load a ZIP
	 * from the FS.  Unload drivers in reverse order. */
	for( int i = g_Drivers.size()-1; i >= 0; --i )
		delete g_Drivers[i].driver;
}

CString LoadedDriver::GetPath( CString path )
{
	if( base.size() == 0 )
		return path;
	
	/* Map all slashes to forward slash. */
	path.Replace( "\\", "/" );

	if( path.Left( base.size() ).CompareNoCase( base ) )
		return ""; /* no match */

	return path.Right( path.size() - base.size() );
}


#include "RageFileDriverDirect.h"
void RageFileManager::AddFS( CString Type, CString Root, CString Base )
{
	if( Base.size() && Base.Right(1) != "/" )
		Base += '/';
	ASSERT( Root != "" );
	if( Root.Right(1) != "/" )
		Root += '/';

	RageFileDriver *driver = NULL;
	if( !Type.CompareNoCase("DIR") )
	{
		driver = new RageFileDriverDirect( Root );
	}

	if( !driver )
		return;
	LoadedDriver ld;
	ld.driver = driver;
	ld.base = Base;
	g_Drivers.push_back( ld );
}
	
RageFileManager::FileType RageFileManager::GetFileType( const CString &sPath )
{
	for( unsigned i = 0; i < g_Drivers.size(); ++i )
	{
		const CString p = g_Drivers[i].GetPath( sPath );
		if( p.size() == 0 )
			continue;
		FileType ret = g_Drivers[i].driver->GetFileType( p );
		if( ret != TYPE_NONE )
			return ret;
	}

	return TYPE_NONE;
}


int RageFileManager::GetFileSizeInBytes( const CString &sPath )
{
	for( unsigned i = 0; i < g_Drivers.size(); ++i )
	{
		const CString p = g_Drivers[i].GetPath( sPath );
		if( p.size() == 0 )
			continue;
		int ret = g_Drivers[i].driver->GetFileSizeInBytes( p );
		if( ret != -1 )
			return ret;
	}

	return -1;
}

int RageFileManager::GetFileModTime( const CString &sPath )
{
	for( unsigned i = 0; i < g_Drivers.size(); ++i )
	{
		const CString p = g_Drivers[i].GetPath( sPath );
		if( p.size() == 0 )
			continue;
		int ret = g_Drivers[i].driver->GetFileModTime( p );
		if( ret != -1 )
			return ret;
	}

	return -1;

}

/* Used only by RageFile: */
RageFileObj *RageFileManager::Open( const CString &sPath, RageFile::OpenMode mode, RageFile &p, int &err )
{
	err = ENOENT;

	/* XXX: WRITE logic */
	for( unsigned i = 0; i < g_Drivers.size(); ++i )
	{
		const CString path = g_Drivers[i].GetPath( sPath );
		if( path.size() == 0 )
			continue;
		int error;
		RageFileObj *ret = g_Drivers[i].driver->Open( path, mode, p, error );
		if( ret )
			return ret;

		/* ENOENT (File not found) is low-priority: if some other error
		 * was reported, return that instead. */
		if( error != ENOENT )
			err = error;
	}

	return NULL;
}

bool RageFileManager::IsAFile( const CString &sPath ) { return GetFileType(sPath) == TYPE_FILE; }
bool RageFileManager::IsADirectory( const CString &sPath ) { return GetFileType(sPath) == TYPE_DIR; }
bool RageFileManager::DoesFileExist( const CString &sPath ) { return GetFileType(sPath) != TYPE_NONE; }

