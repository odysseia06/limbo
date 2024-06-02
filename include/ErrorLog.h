#pragma once
#include "LObject.h"
#include <exception>
#include <string>
#include <sstream>
#include <iostream>
#include <time.h>
#include <iomanip>
#include <fstream>
#include <exception> 

#ifndef THROW_EXCEPTION
#define THROW_EXCEPTION(ErrorNum, ErrorDesc) throw LException(ErrorNum, ErrorDesc, __FILE__, __LINE__);
#endif

class LException : public std::exception {
private:
protected:
public:
	int m_ErrorNumber;
	std::string m_ErrorDesc;
	std::string m_SrcFileName;
	int m_LineNumber;
	std::string m_ErrText;
	//Override std::exception::what
	//Returns string featuring: Error Number, Error Desc, Src File, Line Number
	const char* what() const;
	LException(int ErrorNumber, std::string ErrorDesc, std::string SrcFileName, int LineNumber);
	~LException() throw() {}
};

class LErrorLogManager : public LObject {
private:
protected:
	LErrorLogManager();
	virtual ~LErrorLogManager() {}
	static LErrorLogManager m_ErrorManager;
public:
	static LErrorLogManager* GetErrorManager();
	//Log File Buffer
	std::stringstream m_LogBuffer;
	//Creates a log file
	void create(std::string Filename);
	//Commits contents to file
	void flush();
	/*Closes file*/
	void close();
	//Logs an exception to the log file
	void logException(LException e);
	//Gets the time as string. Can be used for
	//recording the time of an error in the log
	std::string getTimeString();
	//Handle to log file
	std::ofstream m_LogFile;
};