// CleanProject.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>

#include <list>
#include <string>

std::list<std::string> dir_list;
std::list<std::string> ext_list;

std::list<std::string> del_folder;
std::list<std::string> del_file;

std::string work_path;

bool bUsage= false;

void Usage()
{
	printf("\t/D:\n");
	printf("\t\tdirectory to project to clean\n");
	printf("\n/D:sample");

	printf("\t/H\n");
	printf("\t\tprint usage message\n");
	printf("\n");

	printf("\t/RD:\n");
	printf("\t\tdirectory to remove\n");
	printf("\n/RD:Debug");

	printf("\t/RF:\n");
	printf("\t\tfile extension to remove\n");
	printf("\n/RF:*.sdf");

}

bool Parse(int argc, _TCHAR* argv[])
{
	int argument_count = 0;

	for( int i=1; i < argc; i ++ )
	{
		if ( _strnicmp( argv[i], "/D:", 3 ) == 0 )
		{
			work_path = argv[i] + 3 ;
			argument_count ++;
			continue;
		}

		if ( _strnicmp( argv[i], "/H", 2 ) == 0 )
		{
			return false;
		}

		if ( _strnicmp( argv[i], "/RD:", 3 ) == 0 )
		{
			std::string dir = argv[i] + 4;
			dir_list.push_back(dir);
			argument_count ++;
			continue;
		}

		if ( _strnicmp( argv[i], "/RF:*", 3 ) == 0 )
		{
			std::string ext = argv[i] + 5;
			ext_list.push_back(ext);
			argument_count ++;
			continue;
		}
	}

	return (argument_count != 0 );
}


void CleanProjectList(std::string folder)
{

	std::string search_pattern = folder + "\\*.*";
	printf("CleanProjectList::%s\n", folder.c_str() );

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	hFind = ::FindFirstFile(search_pattern.c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		printf ("FindFirstFile failed (%d)\n", GetLastError());
		return;
	} 

	do
	{
		if ( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			if ( _strnicmp( FindFileData.cFileName, "." , 1 ) == 0 )
				continue ;
			if ( _strnicmp( FindFileData.cFileName, "..", 2 ) == 0 )
				continue ;

			bool bFound = false;
			for( std::list<std::string>::iterator i = dir_list.begin(); i != dir_list.end(); ++ i )
			{
				std::string delete_folder = (*i);

				if ( delete_folder == FindFileData.cFileName )
				{
					std::string child_folder = folder + "\\" + FindFileData.cFileName;
					del_folder.push_back( child_folder );
					bFound = true;
					break;
				}
			}

			if ( !bFound)
			{
				std::string child_folder = folder + "\\" + FindFileData.cFileName;
				CleanProjectList( child_folder );
			}
		}
		else
		{
			bool bFound = false;
			for( std::list<std::string>::iterator i = ext_list.begin(); i != ext_list.end(); ++ i )
			{
				std::string ext_name = (*i);
				int file_name_length = strlen( FindFileData.cFileName );
				int ext_length = (*i).length();

				if ( file_name_length >= ext_length ) 
				{
					if ( _strnicmp( (*i).c_str(), FindFileData.cFileName + file_name_length - ext_length,ext_length) == 0 )
					{
						std::string child_file = folder + "\\" + FindFileData.cFileName;
						del_file.push_back( child_file );

						bFound = true;
						break;
					}
				}
			}
		}

	} 
	while( ::FindNextFile( hFind, &FindFileData) != 0 );

     ::FindClose(hFind);

}

int DeleteDirectory(std::string root_directory, bool bDeleteSubdirectories = true)
{
	bool            bSubdirectory = false;       // Flag, indicating whether
                                               // subdirectories have been found
	HANDLE          hFile;                       // Handle to directory

	WIN32_FIND_DATA FindFileData;

	std::string file_name;                
	std::string search_pattern = root_directory + "\\*.*";
	hFile = ::FindFirstFile(search_pattern.c_str(), &FindFileData);

	if(hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if( FindFileData.cFileName[0] != '.')
			{
				file_name.erase();
				file_name = root_directory + "\\" + FindFileData.cFileName;

				if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if(bDeleteSubdirectories)
					{
						// Delete subdirectory
						int iRC = DeleteDirectory(file_name, bDeleteSubdirectories);
						if(iRC)
							return iRC;
					}
					else
						bSubdirectory = true;
				}
				else
				{
					// Set file attributes
					if(::SetFileAttributes(file_name.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
						return ::GetLastError();

					// Delete file
					if(::DeleteFile(file_name.c_str()) == FALSE)
						return ::GetLastError();
				}
			}

		} while(::FindNextFile(hFile, &FindFileData) == TRUE);

		::FindClose(hFile);

		DWORD dwError = ::GetLastError();

		if(dwError != ERROR_NO_MORE_FILES)
			return dwError;
		else
		{
			if(!bSubdirectory)
			{
				// Set directory attributes
				if(::SetFileAttributes(root_directory.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
					return ::GetLastError();

				// Delete directory
				if(::RemoveDirectory(root_directory.c_str()) == FALSE)
					return ::GetLastError();
			}
		}
	}

	return 0;
}


int _tmain(int argc, _TCHAR* argv[])
{
	printf("executable = %s\n", argv[0] );

	std::string root_path = argv[0];
	int pos = root_path.rfind("\\");

	if ( pos == -1 )
		root_path = ".";
	else
		root_path = root_path.substr(0, pos );

	if ( ! Parse(argc, argv ) )
	{
		Usage();
		return 0;
	}

	if ( work_path.length() == 0  )
		work_path = root_path;

	printf("work path = %s\n", work_path.c_str() );
	CleanProjectList( work_path );

	for(std::list<std::string>::iterator i = del_folder.begin(); i != del_folder.end(); ++ i )
	{
		printf("\tdelete folder ... %s\n", (*i).c_str() );
		DeleteDirectory( (*i), true );
	}

	for(std::list<std::string>::iterator i = del_file.begin(); i != del_file.end(); ++ i )
	{
		printf("\tdelete file ... %s\n", (*i).c_str() );
		::DeleteFile( (*i).c_str() );
	}

	return 0;
}

