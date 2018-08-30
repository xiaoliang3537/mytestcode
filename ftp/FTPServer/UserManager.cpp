/****************************************************************/
/*																*/
/*  UserManager.cpp												*/
/*																*/
/*  Implementation of the CUserManager class.					*/
/*																*/
/*  Programmed by Pablo van der Meer							*/
/*  Based partially on and inspired by FileZilla Server.		*/
/*																*/
/*  Copyright Pablo Software Solutions 2002						*/
/*	http://www.pablovandermeer.nl								*/
/*																*/
/*  Last updated: 21 july 2002									*/
/*																*/
/****************************************************************/


#include "stdafx.h"
//#include "FTPserverApp.h"
#include "UserManager.h"

//extern CFTPServerApp theApp;

IMPLEMENT_SERIAL(CDirectory, CObject, 1)

CDirectory::CDirectory()
{
}

CDirectory::~CDirectory()
{
}

void CDirectory::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// 'store' data
		ar << m_strDir;
		ar << m_strAlias;
		ar << m_bAllowDownload;
		ar << m_bAllowUpload;
		ar << m_bAllowRename;
		ar << m_bAllowDelete;
		ar << m_bAllowCreateDirectory;
		ar << m_bIsHomeDir;
	}
	else
	{
		// 'load' data
		ar >> m_strDir;
		ar >> m_strAlias;
		ar >> m_bAllowDownload;
		ar >> m_bAllowUpload;
		ar >> m_bAllowRename;
		ar >> m_bAllowDelete;
		ar >> m_bAllowCreateDirectory;
		ar >> m_bIsHomeDir;
	}
}


template <> void AFXAPI SerializeElements <CDirectory> (CArchive& ar, CDirectory* pNewDirectories, int nCount)
{
    for (int i = 0; i < nCount; i++, pNewDirectories++)
    {
        // Serialize each CDirectory object
        pNewDirectories->Serialize(ar);
    }
}


/* Copy-constructor */
CDirectory::CDirectory(const CDirectory &dir)
{
	m_strDir = dir.m_strDir;
	m_strAlias = dir.m_strAlias;
	m_bAllowDownload = dir.m_bAllowDownload;
	m_bAllowUpload = dir.m_bAllowUpload;
	m_bAllowRename = dir.m_bAllowRename;
	m_bAllowDelete = dir.m_bAllowDelete;
	m_bAllowCreateDirectory = dir.m_bAllowCreateDirectory;
	m_bIsHomeDir = dir.m_bIsHomeDir;
}

/* = operator definition */
CDirectory& CDirectory::operator=(const CDirectory &dir)
{
	if (&dir != this)
	{
		m_strDir = dir.m_strDir;
		m_strAlias = dir.m_strAlias;
		m_bAllowDownload = dir.m_bAllowDownload;
		m_bAllowUpload = dir.m_bAllowUpload;
		m_bAllowRename = dir.m_bAllowRename;
		m_bAllowDelete = dir.m_bAllowDelete;
		m_bAllowCreateDirectory = dir.m_bAllowCreateDirectory;
		m_bIsHomeDir = dir.m_bIsHomeDir;
	}
	return *this;
}


IMPLEMENT_SERIAL(CUser, CObject, 1)

CUser::CUser()
{
	m_bAccountDisabled = FALSE;
}

CUser::~CUser()
{
}

void CUser::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// 'store' data
		ar << m_strName;
		ar << m_strPassword;
		ar << m_bAccountDisabled;
	}
	else
	{
		// 'load' data
		ar >> m_strName;
		ar >> m_strPassword;
		ar >> m_bAccountDisabled;
	}
	// serialize directories
	m_DirectoryArray.Serialize(ar);
}


/* Copy-constructor */
CUser::CUser(const CUser &user)
{
	m_strName = user.m_strName;
	m_strPassword = user.m_strPassword;
	m_bAccountDisabled = user.m_bAccountDisabled;
	for (int i=0; i < user.m_DirectoryArray.GetSize(); i++)
    {
        CDirectory it = user.m_DirectoryArray[i];
        m_DirectoryArray.Add(it);
    }
		
}

/* = operator definition */
CUser& CUser::operator=(const CUser &user)
{
	if (&user != this)
	{
		m_strName = user.m_strName;
		m_strPassword = user.m_strPassword;
		m_bAccountDisabled = user.m_bAccountDisabled;
		for (int i=0; i < user.m_DirectoryArray.GetSize(); i++)
        {
            CDirectory it = user.m_DirectoryArray[i];
            m_DirectoryArray.Add(it);
        }
			
	}
	return *this;
}



CUserManager::CUserManager()
{
	GetAppDir(m_strFilename);
	m_strFilename += "users.dat";
}

CUserManager::~CUserManager()
{

}


/********************************************************************/
/*																	*/
/* Function name : Serialize										*/
/* Description   : Call this function to store/load the user data	*/
/*																	*/
/********************************************************************/
BOOL CUserManager::Serialize(BOOL bStoring)
{
	static const TCHAR* lpszSignature = _T("Pablo Software Solutions - StoreObject");

	CFile file;

	if (file.Open(m_strFilename, bStoring ? CFile::modeWrite|CFile::modeCreate : CFile::modeRead))
	{
		TRY
		{
			CString str; 
			CArchive ar(&file, bStoring ? CArchive::store : CArchive::load);
			
			if (bStoring)
			{
				// save signature
				ar << CString(lpszSignature);

				// Save the changed user details
				for (int i=0; i < m_UserArray.GetSize(); i++)
				{
					m_UserArray[i].Serialize(ar);
				}

				ar.Flush();
			}
			else
			{
				// load signature
				ar >> str;
				// if this the file we are looking for ?
				if (str.Compare(lpszSignature) == 0)
				{
					int nCount=0;

					while(!ar.IsBufferEmpty())
					{
						CUser user;

						// get user data
						user.Serialize(ar);
						
						// add user to array
						m_UserArray.Add(user);
					}
				}
			}
			ar.Close();
			file.Close();
		}
		CATCH_ALL(e)
		{
			// catch all exceptions that might happen ...
			return FALSE;
		}
		END_CATCH_ALL
	}
	return TRUE;
}


/********************************************************************/
/*																	*/
/* Function name : ConvertPathToLocal								*/
/* Description   : Convert relative path to local path				*/
/*																	*/
/********************************************************************/
BOOL CUserManager::ConvertPathToLocal(LPCTSTR lpszUser, CString &strDirectoryIn, CString &strDirectoryOut)
{
	CUser user;
	if (!GetUser(lpszUser, user))
	{
		// user not valid
		return FALSE;
	}

	CStringList partList;
	CString strSub;
	int nCount=0;

	// split path in parts
	while(AfxExtractSubString(strSub, strDirectoryIn, nCount++, '/'))
	{
		if (!strSub.IsEmpty())
			partList.AddTail(strSub);
	}
	
	// search for home directory
	for (int i=0; i<user.m_DirectoryArray.GetSize(); i++)
	{
		if (user.m_DirectoryArray[i].m_bIsHomeDir)
		{
			CString strHomeDir = user.m_DirectoryArray[i].m_strDir;
			while(!partList.IsEmpty())
			{
				CString strPart = partList.GetHead();
				partList.RemoveHead();

				CString strCheckDir;
				
				if (strPart == "..")
				{
					// go back one level
					int nPos = strHomeDir.ReverseFind('\\');
					if (nPos != -1)
					{
						strCheckDir = strHomeDir.Left(nPos);
					}
				}
				else
				{
					strCheckDir = strHomeDir + "\\" + strPart;
				}
			
				// does directory exist ?
				if (FileExists(strCheckDir, TRUE))
				{
					strHomeDir = strCheckDir;
				}
				else
				// does file exist ?
				if (FileExists(strCheckDir, FALSE))
				{
					strHomeDir = strCheckDir;
				}
				else
				{
					BOOL bFound = FALSE;

					// virtual directories exist only in the root
					if (strHomeDir == user.m_DirectoryArray[i].m_strDir)
					{
						// maybe it's a virtual directory
						for (int j=0; j<user.m_DirectoryArray.GetSize(); j++)
						{
							if (i != j && (user.m_DirectoryArray[j].m_strAlias.CompareNoCase(strPart) == 0))
							{
								bFound = TRUE;
								strHomeDir = user.m_DirectoryArray[j].m_strDir; 
								break;
							}
						}
					}
					if (!bFound)
					{
						// directory not found
						return FALSE;
					}
				}
			}
			
			// successfully converted directory
			strDirectoryOut = strHomeDir;
			return TRUE;
		}
	}
	// no home directory found
	return FALSE;
}


/********************************************************************/
/*																	*/
/* Function name : CheckAccessRights								*/
/* Description   : Check if user has access to specified directory.	*/
/*																	*/
/********************************************************************/
BOOL CUserManager::CheckAccessRights(LPCTSTR lpszUser, LPCTSTR lpszDirectory, int nOption)
{
	CUser user;
	if (!GetUser(lpszUser, user))
	{
		// user not valid
		return FALSE;
	}

	// start with full path
	CString strCheckDir = lpszDirectory;

	while(!strCheckDir.IsEmpty())
	{
		// search for a matching part
		for (int i=0; i<user.m_DirectoryArray.GetSize(); i++)
		{
			CString strPath1 = strCheckDir;
			strPath1.TrimRight(_T("\\"));
			CString strPath2 = user.m_DirectoryArray[i].m_strDir;
			strPath2.TrimRight(_T("\\"));

			// found a match ?
			if (strPath1.CompareNoCase(strPath2) == 0)
			{
				// check file access rights
				if (((!user.m_DirectoryArray[i].m_bAllowDownload) && (nOption == FTP_DOWNLOAD)) ||
					((!user.m_DirectoryArray[i].m_bAllowUpload) && (nOption == FTP_UPLOAD)) ||
					((!user.m_DirectoryArray[i].m_bAllowRename) && (nOption == FTP_RENAME)) ||
					((!user.m_DirectoryArray[i].m_bAllowDelete) && (nOption == FTP_DELETE)) ||
					((!user.m_DirectoryArray[i].m_bAllowCreateDirectory) && (nOption == FTP_CREATE_DIR)))
				{
					return FALSE;
				}
				return TRUE;
			}
		}
		int nPos = strCheckDir.ReverseFind('\\');
		if (nPos != -1)
		{
			// strip subdir 
			strCheckDir = strCheckDir.Left(nPos);
		}
		else
		{
			// we're done
			strCheckDir.Empty();
		}
	} 
	// users has no rights to this directory
	return FALSE;
}


/********************************************************************/
/*																	*/
/* Function name : ChangeDirectory									*/
/* Description   : Change to specified directory					*/
/*																	*/
/********************************************************************/
int CUserManager::ChangeDirectory(LPCTSTR lpszUser, CString &strCurrentdir, CString &strChangeTo)
{
	// make unix style
	strChangeTo.Replace(_T("\\"),_T("/"));
	while(strChangeTo.Replace(_T("//"),_T("/")));
	strChangeTo.TrimRight(_T("/"));

	// now looks something like this: 
	// ""				= root
	// "/mydir/apps"	= absolute path
	// "mydir/apps"		= relative path

	if (strChangeTo == _T(""))
	{
		// goto root
		strChangeTo = _T("/");
	}
	else
	{
		// first character '/' ?
		if (strChangeTo.Left(1) != _T("/"))
		{ 
			// client specified a path relative to their current path
			strCurrentdir.TrimRight(_T("/"));
			strChangeTo = strCurrentdir + _T("/") + strChangeTo;
		}
	}
	
	// goto parent directory
	if (strChangeTo.Right(2) == "..")
	{
		strChangeTo.TrimRight(_T(".."));
		strChangeTo.TrimRight(_T("/"));
		int nPos = strChangeTo.ReverseFind('/');
		if (nPos != -1)
		{
			strChangeTo = strChangeTo.Left(nPos);
		}
		if (strChangeTo == "")
		{
			// goto root
			strChangeTo = "/";
		}
	}

	// get local path
	CString strLocalPath;
	if (!ConvertPathToLocal(lpszUser, strChangeTo, strLocalPath))
	{
		// unable to convert to local path
		return 2;
	}
	
	// check if user has access right for this directory
	if (!CheckAccessRights(lpszUser, strLocalPath, FTP_DOWNLOAD))
	{
		// user has no permissions
		return 1;
	}

	// everything went successfully
	strCurrentdir = strChangeTo; 
	return 0;
}


/********************************************************************/
/*																	*/
/* Function name : GetDirectoryList									*/
/* Description   : Get directory listing for specfied directory		*/
/*																	*/
/********************************************************************/
int CUserManager::GetDirectoryList(LPCTSTR lpszUser, LPCTSTR lpszDirectory, CString &strResult)
{
	CString strDirectory = lpszDirectory;
	
	// make unix style
	strDirectory.Replace(_T("\\"),_T("/"));
	while(strDirectory.Replace(_T("//"),_T("/")));

	// clear list
	strResult = "";

	CUser user;
	if (!GetUser(lpszUser, user))
	{
		// user not found -> no permissions
		return 1;
	}

	CString strLocalPath;
	if (!ConvertPathToLocal(lpszUser, strDirectory, strLocalPath))
	{
		// unable to convert to local path
		return 2;
	}

	// check if user has access right for this directory
	if (!CheckAccessRights(lpszUser, strLocalPath, FTP_DOWNLOAD))
	{
		// user has no permissions, to display this directory
		return 1;
	}

	CFileFind find;
	BOOL bFound = FALSE;

	// check if it's a directory
	if ((GetFileAttributes(strLocalPath) & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		bFound = find.FindFile(strLocalPath + "\\*.*");
	}
	else
	{
		// it's a file
		bFound = find.FindFile(strLocalPath);
	}

	while (bFound)
	{
		bFound = find.FindNextFile();
		
		// skip "." and ".." 
		if (find.IsDots())
			continue;

		// permissions
		if (find.IsDirectory())
			strResult += "drwx------";
		else
			strResult += "-rwx------";

		// groups
		strResult += " 1 user group ";
		// file size
		CString strLength;
		strLength.Format(_T("%d"), find.GetLength());
		CString strFiller = "              ";
		strResult += strFiller.Left(strFiller.GetLength() - strLength.GetLength());
		strResult += strLength;
		// file date
		strResult += GetFileDate(find);
		// file name
		strResult += find.GetFileName();
		// end of line
		strResult += _T("\r\n");
	}
	
	// if it's the root dir also show virtual directories
	for (int i=0; i<user.m_DirectoryArray.GetSize(); i++)
	{
		if (user.m_DirectoryArray.GetAt(i).m_bIsHomeDir)
		{
			CString strPath = user.m_DirectoryArray.GetAt(i).m_strDir;
			strPath.TrimRight('\\');
				
			if (strLocalPath.CompareNoCase(strPath) == 0)
			{
				for (int j=0; j<user.m_DirectoryArray.GetSize(); j++)
				{
					if (i != j && user.m_DirectoryArray.GetAt(j).m_strAlias != "")
					{
						if (find.FindFile(user.m_DirectoryArray.GetAt(j).m_strDir))
						{
							find.FindNextFile();

							strResult += "drwx------";
							// groups
							strResult += " 1 user group ";
							strResult += "             0";
							// file date
							strResult += GetFileDate(find);
							// file name
							strResult += user.m_DirectoryArray.GetAt(j).m_strAlias;
							// end of line
							strResult += "\r\n"; 
						}
					}
				}
			}
			break;
		}
	}
	return 0;
}


/********************************************************************/
/*																	*/
/* Function name : FileExists										*/
/* Description   : Check if file or directory exists				*/
/*																	*/
/********************************************************************/
BOOL CUserManager::FileExists(LPCTSTR lpszFileName, BOOL bIsDirCheck)
{
	// A quick'n'easy way to see if a file exists.
	DWORD dwAttributes = GetFileAttributes(lpszFileName);
    if (dwAttributes == 0xFFFFFFFF)
        return FALSE;

	if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		if (bIsDirCheck)
			return TRUE;
		else
			return FALSE;
	}
	else
	{
		if (!bIsDirCheck)
			return TRUE;
		else
			return FALSE;
	}
}


/********************************************************************/
/*																	*/
/* Function name : CheckFileName									*/
/* Description   : Check if filename is valid and if user has		*/
/*				   access rights.									*/
/*																	*/
/********************************************************************/
int CUserManager::CheckFileName(LPCTSTR lpszUser, CString strFilename, CString strCurrentdir, int nOption, CString &strResult)
{
	// make unix style
	strFilename.Replace(_T("\\"), _T("/"));
	while(strFilename.Replace(_T("//"), _T("/")));
	strFilename.TrimRight(_T("/"));
	
	if (strFilename == _T(""))
	{
		// no file name
		return 2;
	}

	// append filename to directory
	CString strDirectory = strCurrentdir;

	// client has specified complete path 
	int nPos = strFilename.ReverseFind('/');
	if (nPos != -1)
	{
		strDirectory = strFilename.Left(nPos);
		if (strDirectory == _T(""))
			strDirectory = _T("/");
		strFilename = strFilename.Mid(nPos+1);
	}

	// get local path
	CString strLocalPath;
	if (!ConvertPathToLocal(lpszUser, strDirectory, strLocalPath))
	{
		// directory does not exist
		return 2;
	}

	// create the complete path
	strResult = strLocalPath + "\\" + strFilename;

	if ((nOption != FTP_UPLOAD) && !FileExists(strResult, FALSE))
	{
		// file does not exist
		return 2;
	}

	// return relative path
	if (nOption == FTP_LIST)
	{
		strResult = strCurrentdir;
		strResult.TrimRight('/');
		strResult += "/" + strFilename;
		return 0;
	}

	// check file access rights
	if (!CheckAccessRights(lpszUser, strLocalPath, nOption))
	{
		// user has no permissions, to execute specified action
		return 1;
	}
	// everything is ok
	return 0;
}


/********************************************************************/
/*																	*/
/* Function name : CheckDirectory									*/
/* Description   : Check if directory exists and if user has		*/
/*				   appropriate permissions.							*/
/*																	*/
/********************************************************************/
int CUserManager::CheckDirectory(LPCTSTR lpszUser, CString strDirectory, CString strCurrentdir, int nOption, CString &strResult)
{
	// make unix compatible
	strDirectory.Replace(_T("\\"),_T("/"));
	while(strDirectory.Replace(_T("//"),_T("/")));
	strDirectory.TrimRight(_T("/"));
	
	if (strDirectory == _T(""))
	{
		// no directory
		return 2;
	}
	else
	{
		// first character '/' ?
		if (strDirectory.Left(1) != _T("/"))
		{ 
			// client specified a path relative to their current path
			strCurrentdir.TrimRight(_T("/"));
			strDirectory = strCurrentdir + "/" + strDirectory;
		}
	}

	// split part into 2 parts
	int nPos = strDirectory.ReverseFind('/');
	if (nPos == -1)
		return 2;

	// get left part of directory
	CString strNode = strDirectory.Left(nPos);
	// root ?
	if (strNode == "")
		strNode = "/";
	// get right part of directory
	strDirectory = strDirectory.Mid(nPos+1);

	CString strLocalPath;

	do
	{
		// does parent directory exist ?
		if ((!ConvertPathToLocal(lpszUser, strNode, strLocalPath)) && (nOption == FTP_CREATE_DIR))
		{ 
			// directory could not be found, maybe one level higher
			int nPos = strNode.ReverseFind('/');
			// no more levels
			if (nPos == -1) 
				return 2;

			strDirectory = strNode.Mid(nPos+1) + "/" + strDirectory;
			strNode = strNode.Left(nPos);
			continue;
		}

		// check directory access rights
		if (!CheckAccessRights(lpszUser, strLocalPath, nOption))
		{
			// user has no permissions, to execute specified action
			return 1;
		}
		
		strNode = strLocalPath;
		break;
	} 
	while (strNode != _T("/"));
	
	strDirectory.Replace(_T("/"),_T("\\"));
	strResult = strNode + _T("\\") + strDirectory;
		
	// check if directory exists
	if (!FileExists(strResult))
		return 2;

	// function successfull
	return 0;
}


/********************************************************************/
/*																	*/
/* Function name : GetUser											*/
/* Description   : Get user object for specified username			*/
/*																	*/
/********************************************************************/
BOOL CUserManager::GetUser(LPCTSTR lpszUser, CUser &user)
{
	m_CriticalSection.Lock();
	for (int i=0; i<m_UserArray.GetSize(); i++)
	{
		if (!m_UserArray[i].m_strName.CompareNoCase(lpszUser))
		{
			user = m_UserArray[i];
			m_CriticalSection.Unlock();
			return TRUE;
		}
	}
	m_CriticalSection.Unlock();
	return FALSE;
}


/********************************************************************/
/*																	*/
/* Function name : GetUserList										*/
/* Description   : Make copy of user array							*/
/*																	*/
/********************************************************************/
void CUserManager::GetUserList(CArray<CUser, CUser&>&array)
{
	m_CriticalSection.Lock();
	for (int i=0; i<m_UserArray.GetSize();i++)
	{
		array.Add(m_UserArray[i]);
	}
	m_CriticalSection.Unlock();
}


/********************************************************************/
/*																	*/
/* Function name : UpdateUserList									*/
/* Description   : Update user array								*/
/*																	*/
/********************************************************************/
void CUserManager::UpdateUserList(CArray<CUser, CUser&>&array)
{
	m_CriticalSection.Lock();
	m_UserArray.RemoveAll();
	for (int i=0; i<array.GetSize();i++)
	{
		m_UserArray.Add(array[i]);
	}
	m_CriticalSection.Unlock();
	Serialize(TRUE);
}


/********************************************************************/
/*																	*/
/* Function name : GetFileDate										*/
/* Description   : return file date in unix style					*/
/*																	*/
/********************************************************************/
CString CUserManager::GetFileDate(CFileFind &find)
{
	CString strResult;

	CTime time = CTime::GetCurrentTime();

	find.GetLastWriteTime(time);

	CTimeSpan timeSpan = CTime::GetCurrentTime() - time;

	if (timeSpan.GetDays() > 356)
	{
		strResult = time.Format(" %b %d %Y ");
	}
	else
	{
		strResult.Format(_T(" %s %02d:%02d "), time.Format("%b %d"), time.GetHour(), time.GetMinute());
	}

	return strResult;
}


